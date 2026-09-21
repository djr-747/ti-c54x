#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_STORE_COMPUTE_COMMIT_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_STORE_COMPUTE_COMMIT_H

/*
 * Module contract
 *
 * Owns: the READY-path commit for one dual-indirect WORD16
 * store-compute pair. It runs the staged single-read
 * completion on the X-side word, applies the resulting
 * accumulator effect, and then publishes both validated
 * post-access AR commits and the sequential PC atomically.
 * Does not own: request preparation, endpoint issue/resume,
 * pair lifecycle, cancellation/reset, opcode semantics,
 * address arithmetic, endpoint storage, timing, CPU lifecycle,
 * observers, diagnostics, allocation, I/O, or global state. No
 * architectural field is written until completion staging
 * succeeds.
 */

#include <stdint.h>

#include "memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ti_c54x_arch_state;
struct ti_c54x_accumulator_state;
struct ti_c54x_t_register_state;

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_dual_store_compute_commit_ready(
    ti_c54x_memory_instruction_transaction *transaction,
    struct ti_c54x_arch_state *state,
    struct ti_c54x_accumulator_state *accumulators,
    struct ti_c54x_t_register_state *t_register,
    uint16_t x_value_word);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_STORE_COMPUTE_COMMIT_H */
