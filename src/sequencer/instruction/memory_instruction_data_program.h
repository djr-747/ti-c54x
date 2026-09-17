#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DATA_PROGRAM_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DATA_PROGRAM_H

/*
 * Module contract
 *
 * Owns: the root DATA+PROGRAM WORD16-read seam, including deferred
 * two-request preparation, two-ID reservation, endpoint issue and
 * resume, pair completion, the optional Smem delay write-back
 * (third ID through the reused child with the pair held COMPLETE),
 * atomic Smem address-commit publication, and terminal
 * cancellation/reset.
 * Does not own: opcode decode or semantics, address arithmetic, CPU
 * lifecycle, endpoint storage, timing, observers, diagnostics,
 * allocation, I/O, or global state. All candidate and
 * architectural inputs are borrowed for one call; accepted values
 * remain in caller-owned transaction storage.
 */

#include <stdbool.h>

#include "memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ti_c54x_memory_instruction_data_program_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction);

/* Resolve the pair snapshot, select DATA_PROGRAM_WORD16_READ, and
 * freeze its copied leg payload, returning the prepare status for
 * the shared chain. Resolution lives here (not in the shared
 * prepare chain) so the root transaction owner stays under its
 * source-size budget. */
ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_data_program_prepare_candidate(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_data_program_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_data_program_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_data_program_reset(
    ti_c54x_memory_instruction_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DATA_PROGRAM_H */
