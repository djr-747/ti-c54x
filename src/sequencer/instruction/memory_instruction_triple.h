#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRIPLE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRIPLE_H

/*
 * Module contract
 *
 * Owns: the root dual-data-plus-program WORD16-read seam,
 * including deferred three-request preparation, three-ID
 * reservation, endpoint issue and resume, triple completion,
 * atomic address-commit publication, and terminal
 * cancellation/reset.
 * Does not own: opcode decode or semantics, address arithmetic, CPU
 * lifecycle, endpoint storage, timing, observers, diagnostics, allocation,
 * I/O, or global state. All candidate and architectural inputs are borrowed
 * for one call; accepted values remain in caller-owned transaction storage.
 */

#include <stdbool.h>

#include "memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ti_c54x_memory_instruction_triple_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction);

/* Select DUAL_PROGRAM_TRIPLE, resolve the DATA legs into the copied
 * snapshot, and freeze its copied selector payload. */
bool ti_c54x_memory_instruction_triple_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_triple_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_triple_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_triple_reset(
    ti_c54x_memory_instruction_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRIPLE_H */
