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
 * delayed push kinds, interrupt-take preparation (parked-PC
 * return staging with a caller-owned vector target), and trap
 * vector freezing (K select from the frozen bundle with a live
 * IPTR-derived vector). Does not own:
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

/* Trap vector freeze: derive the 5-bit K select from the frozen
 * one-word bundle, prove the live IPTR is known (unknown IPTR
 * rejects transactionally per the RESET precedent), and publish
 * the K vector (IPTR << 7) + (K << 2) for the commit redirect.
 * Only TRAP-kind WRITE effects with null completions are
 * admitted: a trap mutates no status state. */
bool ti_c54x_memory_instruction_trap_freeze_vector(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint16_t *vector_word_out);

/* Intr vector freeze: as the trap freeze, plus a non-null
 * completion (the INTM-setting take completion the op_intr
 * leaf stages) and proven IFR knownness whenever K names a
 * 16-bit IFR position. */
bool ti_c54x_memory_instruction_intr_freeze_vector(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint16_t *vector_word_out);

/* Dispatch one software-interrupt vector freeze by address
 * kind (TRAP or INTR); any other kind succeeds without
 * touching the output. */
bool ti_c54x_memory_instruction_software_interrupt_freeze_vector(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint16_t *vector_word_out);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_stack_push_deferred_report(
    const ti_c54x_memory_instruction_candidate *candidate,
    ti_c54x_memory_instruction_result *result);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_STACK_PUSH_H */
