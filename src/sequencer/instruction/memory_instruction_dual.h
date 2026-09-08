#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_H

/*
 * Module contract
 *
 * Owns: the root dual-indirect MOD0/MOD0 WORD16-read seam, including
 * deferred two-request preparation, two-ID reservation, endpoint issue and
 * resume, pair completion, and terminal cancellation/reset.
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

bool ti_c54x_memory_instruction_dual_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction);

/* Select DUAL_WORD16_READ and freeze its copied X/Y selector payload. */
bool ti_c54x_memory_instruction_dual_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_dual_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_dual_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_dual_reset(
    ti_c54x_memory_instruction_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_H */
