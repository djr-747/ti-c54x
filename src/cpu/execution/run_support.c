/* Module contract
 * Owns: run-loop result construction, stop/wait mapping, and the
 * deferred-branch adopt/advance/apply plumbing shared by the
 * step/run execution loop.
 * Does not own: CPU lifetime/storage, fetch/decode orchestration,
 * the execution loop itself, bus descriptor validation, lifecycle
 * cancellation, trace assembly, or opcode semantics.
 */

#include "run_support.h"

#include "../memory/pending_work.h"
#include "repeat_activate.h"
#include "sequencer/reset/reset_apply.h"
#include "state/interrupt_flag_register.h"

ti_c54x_run_result ti_c54x_run_result_from_cpu(
    const ti_c54x_cpu *cpu, ti_c54x_stop_reason reason,
    ti_c54x_bus_status bus_status, uint64_t instructions_attempted,
    uint64_t instructions_retired)
{
    ti_c54x_run_result result;

    result.reason = reason;
    result.bus_status = bus_status;
    result.pc_word = cpu->arch_state.pc_word;
    result.opcode_word = cpu->last_fetched_word;
    result.opcode_valid = cpu->last_fetch_valid;
    result.instructions_attempted = instructions_attempted;
    result.instructions_retired = instructions_retired;
    return result;
}

ti_c54x_run_result ti_c54x_run_invalid_result(void)
{
    ti_c54x_run_result result;

    result.reason = TI_C54X_STOP_INVALID_STATE;
    result.bus_status = TI_C54X_BUS_NO_CALLBACK;
    result.pc_word = 0U;
    result.opcode_word = 0U;
    result.opcode_valid = false;
    result.instructions_attempted = 0U;
    result.instructions_retired = 0U;
    return result;
}

ti_c54x_run_result ti_c54x_run_stop_result(
    ti_c54x_cpu *cpu, ti_c54x_stop_reason reason,
    ti_c54x_bus_status bus_status, uint64_t instructions_attempted,
    uint64_t instructions_retired)
{
    ti_c54x_run_result result;

    TI_C54X_CPU_EMIT_TRACE(cpu, TI_C54X_TRACE_STOP);
    result = ti_c54x_run_result_from_cpu(cpu, reason, bus_status,
                                         instructions_attempted,
                                         instructions_retired);
    cpu->execution_active = false;
    return result;
}

ti_c54x_run_result ti_c54x_run_wait_result(
    ti_c54x_cpu *cpu, uint64_t instructions_attempted,
    uint64_t instructions_retired)
{
    ti_c54x_run_result result = ti_c54x_run_result_from_cpu(
        cpu, TI_C54X_STOP_BUS_WAIT, TI_C54X_BUS_OK,
        instructions_attempted, instructions_retired);

    /* BUS_WAIT is a resumable work-unit suspension, not a terminal stop. */
    cpu->execution_active = false;
    return result;
}

ti_c54x_stop_reason ti_c54x_run_bridge_status_to_stop(
    ti_c54x_program_pc_bridge_status status)
{
    switch (status) {
    case TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_CARRIER:
        return TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS;
    case TI_C54X_PROGRAM_PC_BRIDGE_UNRESOLVED_BOUNDARY:
        return TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY;
    case TI_C54X_PROGRAM_PC_BRIDGE_OK:
    case TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT:
    case TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_PROGRAM_WORD_COUNT:
    case TI_C54X_PROGRAM_PC_BRIDGE_INVALID_EFFECT:
    case TI_C54X_PROGRAM_PC_BRIDGE_STALE_SOURCE:
    default:
        return TI_C54X_STOP_INVALID_STATE;
    }
}

bool ti_c54x_run_memory_status_ok(ti_c54x_memory_instruction_status status)
{
    return status == TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

ti_c54x_stop_reason ti_c54x_run_target_status_to_stop(
    ti_c54x_program_pc_target_status status)
{
    switch (status) {
    case TI_C54X_PROGRAM_PC_TARGET_UNSUPPORTED_CARRIER:
        return TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS;
    case TI_C54X_PROGRAM_PC_TARGET_OK:
    case TI_C54X_PROGRAM_PC_TARGET_INVALID_ARGUMENT:
    case TI_C54X_PROGRAM_PC_TARGET_INVALID_EFFECT:
    case TI_C54X_PROGRAM_PC_TARGET_STALE_SOURCE:
    default:
        return TI_C54X_STOP_INVALID_STATE;
    }
}

/* Adopt an armed deferred-branch report from a retired MEMORY root,
 * replacing any pending countdown. The arming instruction consumes no
 * delay-slot words itself. Arming a live instance cannot fail. */
void ti_c54x_run_adopt_deferred_branch(
    ti_c54x_cpu *cpu, const ti_c54x_memory_instruction_result *result)
{
    if (result->deferred_branch_armed) {
        (void)ti_c54x_sequencer_delayed_branch_arm(
            &cpu->delayed_branch, result->deferred_branch_pc_word);
    }
}

/* An inner PC redirection cancels the pending countdown: a non-delayed
 * stack branch or PC_WRITE effect inside the delay slots wins under the
 * documented last-wins rule (TI forbids these outright). */
bool ti_c54x_run_memory_effect_redirects_pc(const ti_c54x_op_effect *effect)
{
    ti_c54x_op_memory_address_kind kind;

    if (effect == NULL || effect->kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    kind = effect->payload.memory.address_spec.kind;
    return kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TAKE_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TRAP_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_INTR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16;
}

/* Maintain the pending countdown after a retirement that armed nothing
 * new. Reports through took_effect whether the deferred target now
 * applies; the caller publishes it to the live PC. A zero word count
 * is rejected without mutation. */
ti_c54x_sequencer_delayed_branch_status ti_c54x_run_advance_delayed_branch(
    ti_c54x_cpu *cpu, uint8_t word_count, bool redirected_pc,
    bool *took_effect)
{
    if (!ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch)) {
        *took_effect = false;
        return TI_C54X_SEQUENCER_DELAYED_BRANCH_OK;
    }
    if (redirected_pc) {
        (void)ti_c54x_sequencer_delayed_branch_disarm(&cpu->delayed_branch);
        *took_effect = false;
        return TI_C54X_SEQUENCER_DELAYED_BRANCH_OK;
    }
    return ti_c54x_sequencer_delayed_branch_consume(
        &cpu->delayed_branch, word_count, took_effect);
}

/* Publish a taken deferred branch to the live PC. The countdown keeps
 * the 16-bit deferred target through the take-effect consume. */
void ti_c54x_run_apply_deferred_branch(ti_c54x_cpu *cpu)
{
    cpu->arch_state.pc_word =
        (ti_c54x_program_word_address)cpu->delayed_branch.deferred_pc_word;
}

bool ti_c54x_run_memory_retirement_tail(
    ti_c54x_cpu *cpu,
    const ti_c54x_memory_instruction_result *result,
    uint8_t retired_word_count,
    bool retired_redirects_pc)
{
    bool took_effect = false;

    if (cpu == NULL || result == NULL) {
        return false;
    }
    ti_c54x_run_adopt_deferred_branch(cpu, result);
    if (!result->deferred_branch_armed) {
        if (ti_c54x_run_advance_delayed_branch(cpu, retired_word_count,
                                               retired_redirects_pc,
                                               &took_effect) !=
            TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
            return false;
        }
        if (took_effect) {
            ti_c54x_run_apply_deferred_branch(cpu);
        }
    }
    /* INTR software interrupts clear the K-indexed IFR bit at
     * retire for K below 16: prepare already proved IFR known
     * and nothing un-knows IFR while the single-instruction
     * transaction is outstanding, so a failure here is
     * unreachable but still transactional. K at 16 or above
     * names no 16-bit IFR position and skips the clear. The
     * frozen candidate still holds the first word: this tail
     * runs before the transaction reset below. */
    if (cpu->instruction_transaction.candidate.effect.kind ==
            TI_C54X_OP_EFFECT_MEMORY &&
        cpu->instruction_transaction.candidate.effect.payload.memory
                .address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_INTR_WORD16) {
        uint16_t first_word =
            cpu->instruction_transaction.candidate.instruction.bundle
                .program_words[0];
        uint8_t k = (uint8_t)(first_word & UINT16_C(0x001F));

        if ((uint16_t)(first_word & UINT16_C(0xFFE0)) !=
                UINT16_C(0xF7C0) ||
            (k < UINT8_C(16) &&
             ti_c54x_arch_state_clear_ifr_bit(
                 &cpu->arch_state,
                 (uint16_t)((uint16_t)1U << k)) !=
                 TI_C54X_ARCH_STATE_OK)) {
            return false;
        }
    }
    if (!ti_c54x_run_memory_status_ok(ti_c54x_cpu_memory_reset_transaction(
            &cpu->instruction_transaction,
            &cpu->bus.data_endpoint))) {
        return false;
    }
    /* Retirement-tail HOLD re-drive for repeated bodies. */
    return ti_c54x_cpu_repeat_body_retired(cpu) !=
           TI_C54X_CPU_REPEAT_RETIRE_INVALID;
}

bool ti_c54x_run_route_pending_repeat_start(
    ti_c54x_cpu *cpu,
    const ti_c54x_memory_instruction_result *memory_result)
{
    ti_c54x_op_effect repeat_effect = {0};
    ti_c54x_cpu_repeat_route_status repeat_status;

    if (!memory_result->repeat_start_pending) {
        return true;
    }
    repeat_effect.kind = TI_C54X_OP_EFFECT_REPEAT_START;
    repeat_effect.payload.repeat_start.operand =
        memory_result->repeat_start_operand;
    repeat_status = ti_c54x_cpu_repeat_route(
        cpu, &repeat_effect, TI_C54X_PROGRAM_PC_BRIDGE_OK,
        cpu->arch_state.pc_word);
    return repeat_status == TI_C54X_CPU_REPEAT_ROUTE_RETIRED;
}

bool ti_c54x_run_retire_reset(ti_c54x_cpu *cpu, uint8_t retired_word_count)
{
    ti_c54x_reset_apply_status reset_status;
    uint16_t reset_pc;
    uint8_t vector;
    bool took_effect = false;

    if (cpu == NULL) {
        return false;
    }
    reset_status = ti_c54x_reset_apply(&cpu->arch_state, &reset_pc);
    if (reset_status != TI_C54X_RESET_APPLY_OK) {
        return false;
    }
    cpu->arch_state.pc_word = reset_pc;
    cpu->repeat_progress = (ti_c54x_repeat_progress){0};
    cpu->repeat_pending = (ti_c54x_repeat_pending_request){0};
    cpu->repeat_body_pc_word = UINT32_C(0);
    cpu->repeat_block_progress = (ti_c54x_repeat_block_progress){0};
    cpu->nmi_pending = false;
    cpu->nmi_vector_word = UINT16_C(0);
    for (vector = UINT8_C(0); vector < UINT8_C(16); ++vector) {
        cpu->interrupt_vectors[vector] = UINT16_C(0);
    }
    cpu->interrupt_vectors_known = UINT16_C(0);
    cpu->take_source_bit = UINT8_C(0);
    cpu->take_source_is_maskable = false;
    return ti_c54x_run_advance_delayed_branch(cpu, retired_word_count,
                                              true, &took_effect) ==
           TI_C54X_SEQUENCER_DELAYED_BRANCH_OK;
}
