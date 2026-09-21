#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRIPLE_COMMIT_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRIPLE_COMMIT_H

/*
 * Module contract
 *
 * Owns: the READY-path commit for one dual-data-plus-program
 * WORD16 read triple. It stages the ordinary complete_triple
 * effect, then publishes both validated post-access AR commits
 * and the sequential PC atomically.
 * Does not own: request preparation, endpoint issue/resume, triple
 * lifecycle, cancellation/reset, opcode semantics, address
 * arithmetic, endpoint storage, timing, CPU lifecycle, observers,
 * diagnostics, allocation, I/O, or global state. No architectural
 * field is written until ordinary-effect staging succeeds.
 */

#include "memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_triple_commit_ready(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state, ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register, uint16_t x_word,
    uint16_t y_word, uint16_t program_word);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRIPLE_COMMIT_H */
