#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_COMMIT_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_COMMIT_H

/*
 * Module contract
 *
 * Owns: the READY-path commit for one dual-indirect MOD0/MOD1/MOD2 WORD16
 * read pair. It stages the ordinary complete_dual effect and then publishes
 * both validated post-access AR commits and the sequential PC atomically:
 * distinct selectors publish two AR updates, a same-selector alias publishes
 * the one X-owned update, and MOD0 sides change nothing. WAIT, FAULT,
 * cancellation, and other nonterminal paths never reach this owner.
 * Does not own: request preparation, endpoint issue/resume, pair lifecycle,
 * cancellation/reset, opcode semantics, address arithmetic, endpoint
 * storage, timing, CPU lifecycle, observers, diagnostics, allocation, I/O,
 * or global state. All inputs are borrowed for one call; the transaction's
 * copied snapshot must already be validated against the mode/selector/
 * address shadows and live state before this call.
 */

#include <stdint.h>

#include "memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_dual_commit_ready(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state, ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register, uint16_t x_word, uint16_t y_word);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_COMMIT_H */
