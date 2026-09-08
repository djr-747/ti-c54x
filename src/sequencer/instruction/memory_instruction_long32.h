#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_LONG32_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_LONG32_H

/*
 * Module contract
 *
 * Owns: the root LONG32 memory-instruction seam, including deferred pair
 * preparation, two-ID reservation, direct endpoint orchestration, copied
 * MSW/LSW completion, and terminal cancellation/reset.
 * Does not own: opcode decode, address arithmetic, CPU lifecycle, endpoint
 * storage, timing, observers, diagnostics, or global state.
 */

#include <stdbool.h>

#include "memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ti_c54x_memory_instruction_long32_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction);

/* Select LONG32 and freeze its copied completion payload during prepare. */
bool ti_c54x_memory_instruction_long32_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_long32_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_long32_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_long32_reset(
    ti_c54x_memory_instruction_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_LONG32_H */
