/*
 * Module contract
 *
 * Owns: interrupt-take preparation for the root memory
 * transaction: the take is the root's second producer beside
 * decoded opcode instructions. See the prepare-take contract
 * below for the frozen sentinel and vector-carrier rules.
 * Does not own: CPU publication, endpoint callbacks, opcode
 * dispatch, take commit, timing, observers, or diagnostics.
 * Transaction storage is borrowed for each call.
 */

#include "memory_instruction_take.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory_instruction_stack_push.h"

/* Interrupt-take preparation: the root's second producer. The
 * take carries no decoded instruction, input snapshot, or
 * sequential bridge plan: the frozen instruction and input hold
 * the zeroed sentinel (no consumer reads take identity from
 * them) and the frozen pc_effect stays zeroed (the take
 * redirects absolutely to the frozen vector). The parked live PC
 * freezes into the staged TOS value through the take push
 * owner, and the device-owned vector freezes into the
 * LONG32-MSW slot the WORD16 path otherwise leaves zero (keyed
 * off the TAKE address kind; see the carrier comment). */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_prepare_take(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint16_t vector_word,
    uint64_t instructions_attempted_before,
    uint64_t instructions_retired_before)
{
    ti_c54x_memory_instruction_candidate candidate = {0};
    ti_c54x_memory_address_resolution resolution = {0};
    ti_c54x_memory_instruction_status push_status;
    uint16_t push_return_value = UINT16_C(0);

    if (transaction == NULL || effect == NULL || state == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE ||
        transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_MEMORY ||
        !ti_c54x_op_memory_effect_is_valid(&effect->payload.memory)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (effect->payload.memory.address_spec.kind !=
        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_TAKE_WORD16) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (instructions_attempted_before == UINT64_MAX ||
        instructions_retired_before == UINT64_MAX) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_COUNTER_OVERFLOW;
    }
    push_status = ti_c54x_memory_instruction_stack_push_prepare_take(
        effect, state, vector_word, &resolution, &push_return_value);
    if (push_status != TI_C54X_MEMORY_INSTRUCTION_STATUS_OK) {
        return push_status;
    }
    candidate.address_resolution = resolution;
    candidate.effect = *effect;
    /* Stage the live parked return address into the frozen copy.
     * The take has no leaf handler to stage it, and the TOS
     * shape check requires the frozen effect value to equal the
     * issued TOS WRITE value. */
    candidate.effect.payload.memory.value_word = push_return_value;
    candidate.instructions_attempted_after =
        instructions_attempted_before + UINT64_C(1);
    candidate.instructions_retired_after = instructions_retired_before +
                                           UINT64_C(1);
    transaction->candidate = candidate;
    transaction->child_kind = TI_C54X_MEMORY_INSTRUCTION_CHILD_WORD16;
    transaction->candidate_rmw_operation =
        candidate.effect.payload.memory.rmw_operation;
    transaction->candidate_rmw_value_word = push_return_value;
    transaction->long32_first_value_msw = vector_word;
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

