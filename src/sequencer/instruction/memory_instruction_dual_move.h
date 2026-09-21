#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_MOVE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_MOVE_H

/*
 * Module contract
 *
 * Owns: root orchestration for one dual-indirect WORD16 move
 * pair (MVDD). It serializes the X READ then the Y WRITE
 * endpoint calls (the Y WRITE template carries the X
 * observation), routes MMR legs synchronously like the
 * Smem-absolute pair precedent, and delegates the READY
 * commit to the shared architecture commit (which publishes
 * both dual AR updates) only after both legs resolve. The
 * second-leg flag rides the free candidate_rmw_operation byte
 * (the dual READ root instead snapshots selectors there); no
 * transaction grows. No architectural field is written until
 * pair completion passes all candidate and live-state checks.
 * Does not own: opcode semantics, address arithmetic, endpoint
 * storage, timing, CPU lifecycle, observers, diagnostics,
 * allocation, I/O, or global state.
 */

#include <stdbool.h>
#include <stdint.h>

#include "memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ti_c54x_bus_word16_endpoint;
struct ti_c54x_arch_state;
struct ti_c54x_accumulator_state;
struct ti_c54x_t_register_state;
struct ti_c54x_memory_instruction_result;

bool ti_c54x_memory_instruction_dual_move_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction);

bool ti_c54x_memory_instruction_dual_move_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_dual_move_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const struct ti_c54x_bus_word16_endpoint *endpoint,
    struct ti_c54x_arch_state *state,
    struct ti_c54x_accumulator_state *accumulators,
    struct ti_c54x_t_register_state *t_register,
    struct ti_c54x_memory_instruction_result *result);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_dual_move_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const struct ti_c54x_bus_word16_endpoint *endpoint);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_dual_move_reset(
    ti_c54x_memory_instruction_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_MOVE_H */
