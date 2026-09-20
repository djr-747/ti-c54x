#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TAKE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TAKE_H

/*
 * Module contract
 *
 * Owns: the interrupt-take preparation entry for the root
 * memory transaction. Takes carry no decoded instruction (see
 * the implementation contract in memory_instruction_take.c).
 * Does not own: CPU publication, endpoint callbacks, opcode
 * dispatch, take commit, timing, observers, or diagnostics.
 * Transaction storage is borrowed for each call.
 */

#include <stdint.h>

#include "sequencer/instruction/memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Interrupt-take preparation: freeze a TAKE-kind WORD16 push
 * candidate from a caller-built take effect, the live state,
 * and the device-owned vector. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_prepare_take(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint16_t vector_word,
    uint64_t instructions_attempted_before,
    uint64_t instructions_retired_before);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TAKE_H */
