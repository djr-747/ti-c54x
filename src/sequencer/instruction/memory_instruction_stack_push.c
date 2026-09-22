/*
 * Module contract
 *
 * Owns: the stack-push preparation implementation described in
 * memory_instruction_stack_push.h.
 * Does not own: CPU publication, endpoint callbacks, opcode dispatch,
 * timing, observers, or diagnostics.
 */

#include "memory_instruction_stack_push.h"

#include <stddef.h>
#include <stdint.h>

#include "sequencer/program_pc_target.h"
#include "state/interrupt_flag_register.h"
#include "state/status/registers.h"

/* Derive the frozen call target for accumulator pushes from the frozen
 * input snapshot: the low 16 bits of the selected accumulator gated on
 * accumulator knownness. */
static bool
accumulator_push_target(const ti_c54x_op_input_snapshot *input,
                        uint8_t select, uint16_t *target)
{
    if (input == NULL || target == NULL) {
        return false;
    }
    if (select == UINT8_C(0)) {
        if (!input->accumulator_a_known) {
            return false;
        }
        *target = (uint16_t)(input->accumulator_a_raw40 & UINT64_C(0xFFFF));
        return true;
    }
    if (select == UINT8_C(1)) {
        if (!input->accumulator_b_known) {
            return false;
        }
        *target = (uint16_t)(input->accumulator_b_raw40 & UINT64_C(0xFFFF));
        return true;
    }
    return false;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_stack_push_prepare(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    ti_c54x_memory_address_resolution *resolution,
    ti_c54x_program_pc_bridge_effect *pc_effect,
    uint16_t *push_return_value)
{
    ti_c54x_program_pc_target_effect call_target = {0};
    const bool push_delayed =
        ti_c54x_op_memory_address_kind_is_delayed_push(
            effect->payload.memory.address_spec.kind);
    const bool push_accumulator =
        ti_c54x_op_memory_address_kind_is_accumulator_push(
            effect->payload.memory.address_spec.kind);
    const bool push_trap =
        effect->payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TRAP_WORD16 ||
        effect->payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_INTR_WORD16;
    uint16_t pmad;
    uint32_t return_address_word;

    if (push_accumulator) {
        /* One-word accumulator pushes carry the target outside the
         * bundle: the low 16 bits of the selected accumulator from
         * the frozen input snapshot, gated on accumulator
         * knownness before any bus issue. The row catalog pins the
         * count; this defense keeps a forged candidate from
         * reading past the frozen bundle. Commit re-derives the
         * same target deterministically from the frozen input, so
         * no frozen field grows. */
        if (instruction->bundle.program_word_count != UINT8_C(1)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        if (!accumulator_push_target(
                input, effect->payload.memory.address_spec.arf, &pmad)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    } else if (push_trap) {
        /* Traps and software interrupts carry no bundle target:
         * the one-word bundle holds only the K vector select,
         * and the K vector freezes separately in the root. The
         * row catalog pins the count; this defense keeps a
         * forged candidate from reading a missing extension
         * word. */
        if (instruction->bundle.program_word_count != UINT8_C(1)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        pmad = UINT16_C(0);
    } else {
        /* Two-word push forms carry pmad in the second bundle word.
         * The row catalog pins the count; this defense keeps a
         * forged candidate from reading past the frozen bundle. */
        if (instruction->bundle.program_word_count != UINT8_C(2)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        pmad = instruction->bundle.program_words[1];
    }
    /* Prove the commit-time pmad target plan before any bus issue.
     * Commit re-plans deterministically from the same frozen pmad and
     * the stable live PC, so the discarded effect is validation only.
     * Traps and software interrupts skip this plan: commit
     * re-plans from the frozen K vector instead. */
    if (!push_trap &&
        ti_c54x_program_pc_target_plan(state->pc_word, pmad,
                                       &call_target) !=
            TI_C54X_PROGRAM_PC_TARGET_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    /* The bridge successor doubles as the pushed return address. Plan
     * it before resolution: the TOS WRITE carries PC+2 while commit
     * redirects PC to pmad. Top-of-memory calls inherit the shared
     * no-wrap sequential boundary. */
    if (ti_c54x_program_pc_bridge_plan(
            state->pc_word, instruction->bundle.program_word_count,
            pc_effect) != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    return_address_word = pc_effect->common_pc_word_after;
    if (push_delayed) {
        /* Delayed calls push PC+4 per SPRU172C §4 CALL[D]
         * ((PC)+4 → TOS with the §4-27 worked example pushing
         * 0029 for CALLD at 0025); one-word delayed
         * accumulator pushes stage PC+3 per §4 CALA[D]. The
         * successor stays the commit-time sequential PC; only
         * the staged TOS value moves by two more words. A
         * return address outside 16-bit program space is
         * malformed: TI code always has two slot words after
         * the call. */
        return_address_word += UINT32_C(2);
        if (return_address_word > UINT32_C(0xFFFF)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    }
    if (ti_c54x_sequencer_resolve_memory_address(
            state, &effect->payload.memory.address_spec,
            effect->payload.memory.access, (uint16_t)return_address_word,
            resolution) != TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    *push_return_value = (uint16_t)return_address_word;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_stack_push_prepare_take(
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint16_t vector_word,
    ti_c54x_memory_address_resolution *resolution,
    uint16_t *push_return_value)
{
    ti_c54x_program_pc_target_effect take_target = {0};
    uint32_t return_address_word;

    if (effect == NULL || state == NULL || resolution == NULL ||
        push_return_value == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    /* Only take effects: the take root producer is the sole
     * freezer of this kind. */
    if (effect->kind != TI_C54X_OP_EFFECT_MEMORY ||
        effect->payload.memory.address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TAKE_WORD16 ||
        effect->payload.memory.access !=
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ||
        effect->payload.memory.rmw_operation !=
            (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE ||
        effect->payload.memory.complete == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    /* The parked live PC is the pushed return address: the take
     * preempts fetch, so the live PC is the next unexecuted
     * program word. A return address outside 16-bit program space
     * is malformed. */
    return_address_word = state->pc_word;
    if (return_address_word > UINT32_C(0xFFFF)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    /* Prove the commit-time vector target plan before any bus
     * issue. Commit re-plans deterministically from the same
     * frozen vector and the stable live PC, so the discarded
     * effect is validation only. */
    if (ti_c54x_program_pc_target_plan(state->pc_word, vector_word,
                                       &take_target) !=
        TI_C54X_PROGRAM_PC_TARGET_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (ti_c54x_sequencer_resolve_memory_address(
            state, &effect->payload.memory.address_spec,
            effect->payload.memory.access, (uint16_t)return_address_word,
            resolution) != TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    *push_return_value = (uint16_t)return_address_word;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_stack_push_deferred_report(
    const ti_c54x_memory_instruction_candidate *candidate,
    ti_c54x_memory_instruction_result *result)
{
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16) {
        /* The two-word count is proved by prepare before any
         * mutation, so the frozen pmad read is exact here. */
        result->deferred_branch_armed = true;
        result->deferred_branch_pc_word =
            candidate->instruction.bundle.program_words[1];
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_DELAYED_WORD16) {
        /* The two-word count is proved by prepare before any
         * mutation, so the frozen pmad read is exact here. */
        if (candidate->instruction.bundle.program_word_count !=
            UINT8_C(2)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        result->deferred_branch_armed = true;
        result->deferred_branch_pc_word =
            candidate->instruction.bundle.program_words[1];
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_FAR_ACCUMULATOR_DELAYED_WORD16) {
        uint16_t acc_target = UINT16_C(0);

        /* The one-word count is proved by prepare before any
         * mutation, so the frozen input target re-derives
         * deterministically here. */
        if (!accumulator_push_target(
                &candidate->input,
                candidate->effect.payload.memory.address_spec.arf,
                &acc_target)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        result->deferred_branch_armed = true;
        result->deferred_branch_pc_word = acc_target;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (candidate->effect.payload.memory.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16) {
        uint16_t acc_target = UINT16_C(0);

        /* The one-word count is proved by prepare before any
         * mutation, so the frozen input target re-derives
         * deterministically here. */
        if (!accumulator_push_target(
                &candidate->input,
                candidate->effect.payload.memory.address_spec.arf,
                &acc_target)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        result->deferred_branch_armed = true;
        result->deferred_branch_pc_word = acc_target;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
}

bool
ti_c54x_memory_instruction_trap_freeze_vector(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint16_t *vector_word_out)
{
    ti_c54x_status_register_field_result iptr;
    uint32_t vector;
    uint16_t first_word;
    uint8_t k;

    if (instruction == NULL || effect == NULL || state == NULL ||
        vector_word_out == NULL) {
        return false;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_MEMORY ||
        effect->payload.memory.address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TRAP_WORD16 ||
        effect->payload.memory.access !=
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ||
        effect->payload.memory.complete != NULL ||
        effect->payload.memory.complete_dual != NULL) {
        return false;
    }
    /* K rides the frozen bundle word (the leaf stages no K
     * payload); the row catalog pins the one-word bundle. */
    if (instruction->bundle.program_word_count != UINT8_C(1)) {
        return false;
    }
    first_word = instruction->bundle.program_words[0];
    if ((uint16_t)(first_word & UINT16_C(0xFFE0)) != UINT16_C(0xF4C0)) {
        return false;
    }
    k = (uint8_t)(first_word & UINT16_C(0x001F));
    iptr = ti_c54x_status_register_read_iptr(&state->status);
    if (iptr.status != TI_C54X_STATUS_REGISTER_OK || !iptr.known) {
        return false;
    }
    /* IPTR is 9 bits; the K vector always fits 16 bits. */
    vector = ((uint32_t)iptr.raw_value << 7U) + ((uint32_t)k << 2U);
    if (vector > UINT32_C(0xFFFF)) {
        return false;
    }
    *vector_word_out = (uint16_t)vector;
    return true;
}

bool
ti_c54x_memory_instruction_intr_freeze_vector(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint16_t *vector_word_out)
{
    ti_c54x_status_register_field_result iptr;
    uint16_t ifr_value = UINT16_C(0);
    uint32_t vector;
    uint16_t first_word;
    uint8_t k;

    if (instruction == NULL || effect == NULL || state == NULL ||
        vector_word_out == NULL) {
        return false;
    }
    /* The completion union holds the INTM-setting take
     * completion here, so only the `complete` arm is read:
     * the dual/triple/long32 names alias the same word. */
    if (effect->kind != TI_C54X_OP_EFFECT_MEMORY ||
        effect->payload.memory.address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_INTR_WORD16 ||
        effect->payload.memory.access !=
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ||
        effect->payload.memory.complete == NULL) {
        return false;
    }
    /* K rides the frozen bundle word (the leaf stages no K
     * payload); the row catalog pins the one-word bundle. */
    if (instruction->bundle.program_word_count != UINT8_C(1)) {
        return false;
    }
    first_word = instruction->bundle.program_words[0];
    if ((uint16_t)(first_word & UINT16_C(0xFFE0)) != UINT16_C(0xF7C0)) {
        return false;
    }
    k = (uint8_t)(first_word & UINT16_C(0x001F));
    iptr = ti_c54x_status_register_read_iptr(&state->status);
    if (iptr.status != TI_C54X_STATUS_REGISTER_OK || !iptr.known) {
        return false;
    }
    /* The retirement tail clears the K-indexed IFR bit for K
     * below 16; prove IFR known now so that clear cannot fail
     * after the push commits. K at 16 or above names no 16-bit
     * IFR position and skips the clear. */
    if (k < UINT8_C(16) &&
        ti_c54x_arch_state_read_ifr(state, &ifr_value) !=
            TI_C54X_ARCH_STATE_OK) {
        return false;
    }
    /* IPTR is 9 bits; the K vector always fits 16 bits. */
    vector = ((uint32_t)iptr.raw_value << 7U) + ((uint32_t)k << 2U);
    if (vector > UINT32_C(0xFFFF)) {
        return false;
    }
    *vector_word_out = (uint16_t)vector;
    return true;
}

bool
ti_c54x_memory_instruction_software_interrupt_freeze_vector(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint16_t *vector_word_out)
{
    ti_c54x_op_memory_address_kind kind;

    if (instruction == NULL || effect == NULL || state == NULL ||
        vector_word_out == NULL) {
        return false;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    kind = effect->payload.memory.address_spec.kind;
    if (kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TRAP_WORD16) {
        return ti_c54x_memory_instruction_trap_freeze_vector(
            instruction, effect, state, vector_word_out);
    }
    if (kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_INTR_WORD16) {
        return ti_c54x_memory_instruction_intr_freeze_vector(
            instruction, effect, state, vector_word_out);
    }
    return true;
}
