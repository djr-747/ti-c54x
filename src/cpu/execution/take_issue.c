/*
 * Module contract
 *
 * Owns: the NMI and maskable take-issue implementation
 * described in take_issue.h: arbitration input collection,
 * frozen take-source bookkeeping for the IFR acknowledge,
 * and the shared root-issue flow.
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
#include "state/interrupt_flag_register.h"
#include "state/mmr_file.h"
#include "state/status/registers.h"
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

/* Acknowledge the frozen take source after its retirement
 * commits: maskable takes clear exactly their IFR bit (TI
 * section 6.10.8 hardware acknowledge), NMI takes clear the
 * pending edge. Unreachable on failure: the bit is single and
 * IFR is known from issue, and nothing un-knows IFR while a
 * take root is outstanding. */
static bool
take_acknowledge(ti_c54x_cpu *cpu)
{
    uint16_t mask;

    if (!cpu->take_source_is_maskable) {
        cpu->nmi_pending = false;
        return true;
    }
    mask = (uint16_t)((uint16_t)1U << cpu->take_source_bit);
    return ti_c54x_arch_state_clear_ifr_bit(&cpu->arch_state, mask) ==
           TI_C54X_ARCH_STATE_OK;
}

bool ti_c54x_cpu_take_note_resumed(ti_c54x_cpu *cpu)
{
    if (cpu == NULL) {
        return false;
    }
    if (cpu->instruction_transaction.candidate.effect.kind ==
            TI_C54X_OP_EFFECT_MEMORY &&
        cpu->instruction_transaction.candidate.effect.payload.memory
                .address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TAKE_WORD16) {
        return take_acknowledge(cpu);
    }
    return true;
}

/* Collect the maskable arbitration inputs from live state,
 * mapping unknown reads to unprovable (no take). */
static ti_c54x_take_maskable_snapshot
collect_maskable_snapshot(const ti_c54x_cpu *cpu)
{
    ti_c54x_take_maskable_snapshot snapshot = {0};
    ti_c54x_status_register_field_result intm;
    uint16_t ifr_value = UINT16_C(0);
    uint16_t imr_value = UINT16_C(0);

    intm = ti_c54x_status_register_read_intm(&cpu->arch_state.status);
    snapshot.intm_known =
        intm.status == TI_C54X_STATUS_REGISTER_OK;
    snapshot.intm_value = (uint8_t)intm.raw_value;
    if (ti_c54x_arch_state_read_ifr(&cpu->arch_state, &ifr_value) ==
        TI_C54X_ARCH_STATE_OK) {
        snapshot.ifr_known = true;
        snapshot.ifr_value = ifr_value;
    }
    if (ti_c54x_mmr_file_read_imr(&cpu->arch_state.mmr_file,
                                  &imr_value) == TI_C54X_MMR_FILE_OK) {
        snapshot.imr_known = true;
        snapshot.imr_value = imr_value;
    }
    snapshot.vectors_known_mask = cpu->interrupt_vectors_known;
    snapshot.repeat_active =
        cpu->repeat_progress.phase ==
            TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE ||
        cpu->repeat_block_progress.phase ==
            TI_C54X_REPEAT_BLOCK_PHASE_ACTIVE;
    snapshot.delayed_branch_armed =
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch);
    return snapshot;
}

/* Shared root-issue flow for an arbitrated take: the frozen
 * source is already stored, the vector already selected. */
static ti_c54x_cpu_take_issue_result
issue_take(ti_c54x_cpu *cpu, uint16_t vector_word,
           uint64_t *instructions_attempted,
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
        vector_word, cpu->instructions_attempted - UINT64_C(1),
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
    /* Acknowledge the frozen source with the retirement: the
     * edge or IFR bit clears before the next bus effect. */
    if (!take_acknowledge(cpu)) {
        armed.outcome = TI_C54X_CPU_TAKE_ISSUE_STOP;
        return armed;
    }
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

ti_c54x_cpu_take_issue_result ti_c54x_cpu_take_issue(
    ti_c54x_cpu *cpu, uint64_t *instructions_attempted,
    uint64_t *instructions_retired)
{
    ti_c54x_take_maskable_snapshot maskable_snapshot = {0};
    ti_c54x_take_maskable_result maskable = {
        .decision = TI_C54X_TAKE_MASKABLE_FETCH,
        .source_bit = UINT8_C(0)};
    ti_c54x_cpu_take_issue_result armed = {
        .outcome = TI_C54X_CPU_TAKE_ISSUE_NOT_ARMED,
        .stop_reason = TI_C54X_STOP_INVALID_STATE,
        .bus_status = TI_C54X_BUS_OK};

    if (cpu == NULL || instructions_attempted == NULL ||
        instructions_retired == NULL) {
        armed.outcome = TI_C54X_CPU_TAKE_ISSUE_STOP;
        return armed;
    }
    /* NMI keeps architectural priority over every maskable. */
    if (ti_c54x_take_arbitrate_nmi(
            cpu->nmi_pending,
            cpu->repeat_progress.phase ==
                    TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE ||
                cpu->repeat_block_progress.phase ==
                    TI_C54X_REPEAT_BLOCK_PHASE_ACTIVE,
            ti_c54x_sequencer_delayed_branch_is_armed(
                &cpu->delayed_branch)) ==
        TI_C54X_TAKE_ARBITRATION_TAKE_NMI) {
        cpu->take_source_bit = UINT8_C(0);
        cpu->take_source_is_maskable = false;
        return issue_take(cpu, cpu->nmi_vector_word,
                          instructions_attempted, instructions_retired);
    }
    maskable_snapshot = collect_maskable_snapshot(cpu);
    maskable = ti_c54x_take_arbitrate_maskable(&maskable_snapshot);
    if (maskable.decision != TI_C54X_TAKE_MASKABLE_TAKE) {
        return armed;
    }
    cpu->take_source_bit = maskable.source_bit;
    cpu->take_source_is_maskable = true;
    return issue_take(cpu, cpu->interrupt_vectors[maskable.source_bit],
                      instructions_attempted, instructions_retired);
}
