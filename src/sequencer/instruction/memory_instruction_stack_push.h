#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_STACK_PUSH_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_STACK_PUSH_H

/*
 * Module contract
 *
 * Owns: stack-push preparation for the root memory transaction: target
 * planning for two-word bundle pushes and one-word accumulator pushes
 * (target from the frozen input snapshot gated on accumulator
 * knownness), return-address staging through the shared bridge
 * successor, TOS resolution, deferred-branch reporting for the
 * delayed push kinds, and interrupt-take preparation (parked-PC
 * return staging with a caller-owned vector target). Does not own:
 * CPU publication, endpoint callbacks, opcode dispatch, timing,
 * observers, or diagnostics. Transaction storage is borrowed for
 * each call.
 */

#include <stdbool.h>
#include <stdint.h>

#include "sequencer/instruction/memory_instruction_transaction.h"

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_stack_push_prepare(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    ti_c54x_memory_address_resolution *resolution,
    ti_c54x_program_pc_bridge_effect *pc_effect,
    uint16_t *push_return_value);

/* Interrupt-take preparation: stage the parked live PC as the
 * pushed return address (not a sequential successor), prove the
 * caller-owned vector target plan before any bus issue, and
 * resolve the TOS WRITE through the shared push contract (old
 * SP - 1 with a PRE_ACCESS SP update, SP unknownness rejecting
 * transactionally). Only TAKE-kind WRITE effects with a
 * completion are admitted. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_stack_push_prepare_take(
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint16_t vector_word,
    ti_c54x_memory_address_resolution *resolution,
    uint16_t *push_return_value);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_stack_push_deferred_report(
    const ti_c54x_memory_instruction_candidate *candidate,
    ti_c54x_memory_instruction_result *result);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_STACK_PUSH_H */
