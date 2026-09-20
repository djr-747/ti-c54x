/*
 * Module contract
 *
 * Owns: the NMI take-issue implementation described in
 * take_issue.h.
 * Does not own: CPU lifetime/storage, fetch/decode
 * orchestration, the execution loop itself, stop/wait
 * construction, arbitration policy, take preparation or
 * commit, buses, timing, observers, or diagnostics.
 */

#include "take_issue.h"

#include <stdbool.h>
#include <stdint.h>

#include "../cpu_internal.h"
#include "../memory/pending_work.h"
#include "isa/ops/op_effect.h"
#include "sequencer/instruction/memory_instruction_take.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/interrupt/take_arbitration.h"
#include "sequencer/interrupt/take_complete.h"
#include "sequencer/repeat/repeat_block.h"
#include "sequencer/repeat/repeat_progress.h"
#include "run_support.h"

/* Map a failed root prepare/resume onto the caller's stop. The
 * overflow and ID-exhaustion failures keep the loop's counter
 * stop; every other failure is an invalid take candidate. */
static ti_c54x_cpu_take_issue_result
take_root_failure(ti_c54x_memory_instruction_status status)
{
    ti_c54x_cpu_take_issue_result result = {
        .outcome = TI_C54X_CPU_TAKE_ISSUE_STOP,
        .stop_reason = TI_C54X_STOP_INVALID_STATE,
        .bus_status = TI_C54X_BUS_OK};

    if (status == TI_C54X_MEMORY_INSTRUCTION_STATUS_COUNTER_OVERFLOW ||
        status == TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED) {
        result.stop_reason = TI_C54X_STOP_COUNTER_OVERFLOW;
    }
    return result;
}

void ti_c54x_cpu_take_note_resumed(ti_c54x_cpu *cpu)
{
    if (cpu == NULL) {
        return;
    }
    if (cpu->instruction_transaction.candidate.effect.kind ==
            TI_C54X_OP_EFFECT_MEMORY &&
        cpu->instruction_transaction.candidate.effect.payload.memory
                .address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TAKE_WORD16) {
        cpu->nmi_pending = false;
    }
}

ti_c54x_cpu_take_issue_result ti_c54x_cpu_take_issue(
    ti_c54x_cpu *cpu, uint64_t *instructions_attempted,
    uint64_t *instructions_retired)
{
    ti_c54x_op_effect take_effect = {
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind =
                    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TAKE_WORD16,
            },
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .value_word = UINT16_C(0),
            .rmw_operation =
                (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            .complete = ti_c54x_take_complete_intm,
        }};
    ti_c54x_memory_instruction_status memory_status;
    ti_c54x_cpu_memory_resume_result memory_resume;
    ti_c54x_memory_instruction_result memory_result = {0};
    ti_c54x_cpu_take_issue_result armed = {
        .outcome = TI_C54X_CPU_TAKE_ISSUE_NOT_ARMED,
        .stop_reason = TI_C54X_STOP_INVALID_STATE,
        .bus_status = TI_C54X_BUS_OK};

    if (cpu == NULL || instructions_attempted == NULL ||
        instructions_retired == NULL) {
        armed.outcome = TI_C54X_CPU_TAKE_ISSUE_STOP;
        return armed;
    }
    if (ti_c54x_take_arbitrate_nmi(
            cpu->nmi_pending,
            cpu->repeat_progress.phase ==
                    TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE ||
                cpu->repeat_block_progress.phase ==
                    TI_C54X_REPEAT_BLOCK_PHASE_ACTIVE,
            ti_c54x_sequencer_delayed_branch_is_armed(
                &cpu->delayed_branch)) !=
        TI_C54X_TAKE_ARBITRATION_TAKE_NMI) {
        return armed;
    }
    if (cpu->instructions_attempted == UINT64_MAX ||
        cpu->instructions_retired == UINT64_MAX) {
        armed.outcome = TI_C54X_CPU_TAKE_ISSUE_STOP;
        armed.stop_reason = TI_C54X_STOP_COUNTER_OVERFLOW;
        return armed;
    }
    /* The take is the attempt: no program word is fetched. */
    cpu->instructions_attempted++;
    (*instructions_attempted)++;
    memory_status = ti_c54x_memory_instruction_transaction_prepare_take(
        &cpu->instruction_transaction, &take_effect, &cpu->arch_state,
        cpu->nmi_vector_word, cpu->instructions_attempted - UINT64_C(1),
        cpu->instructions_retired);
    if (!ti_c54x_run_memory_status_ok(memory_status)) {
        return take_root_failure(memory_status);
    }
    memory_resume = ti_c54x_cpu_memory_resume(
        &cpu->instruction_transaction, &cpu->bus.data_endpoint,
        &cpu->arch_state, &cpu->accumulators, &cpu->t_register);
    if (!ti_c54x_run_memory_status_ok(memory_resume.transaction_status)) {
        return take_root_failure(memory_resume.transaction_status);
    }
    memory_result = memory_resume.result;
    if (memory_result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT) {
        armed.outcome = TI_C54X_CPU_TAKE_ISSUE_WAIT;
        return armed;
    }
    if (memory_result.status == TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT) {
        armed.outcome = TI_C54X_CPU_TAKE_ISSUE_STOP;
        armed.stop_reason = TI_C54X_STOP_BUS_FAULT;
        armed.bus_status = memory_result.bus_status;
        return armed;
    }
    if (memory_result.status != TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
        armed.outcome = TI_C54X_CPU_TAKE_ISSUE_STOP;
        return armed;
    }
    if (memory_result.instructions_retired != UINT64_C(1) ||
        cpu->instructions_retired == UINT64_MAX) {
        armed.outcome = TI_C54X_CPU_TAKE_ISSUE_STOP;
        armed.stop_reason = TI_C54X_STOP_COUNTER_OVERFLOW;
        return armed;
    }
    cpu->instructions_retired++;
    (*instructions_retired)++;
    cpu->nmi_pending = false;
    /* The take consumes no program words and the countdown is
     * disarmed by arbitration above; the shared tail honors a
     * redirect that cannot be armed here. */
    if (!ti_c54x_run_memory_retirement_tail(
            cpu, &memory_result, UINT8_C(0),
            ti_c54x_run_memory_effect_redirects_pc(&take_effect))) {
        armed.outcome = TI_C54X_CPU_TAKE_ISSUE_STOP;
        return armed;
    }
    armed.outcome = TI_C54X_CPU_TAKE_ISSUE_CONTINUE;
    return armed;
}
