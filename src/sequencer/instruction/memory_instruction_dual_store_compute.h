#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_STORE_COMPUTE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_STORE_COMPUTE_H

/*
 * Module contract
 *
 * Owns: root orchestration for one dual-indirect WORD16
 * store-compute pair (parallel ST||ADD/SUB/MAC/MAS/MPY). It
 * serializes the X READ then the Y WRITE endpoint calls (the Y
 * WRITE template carries the leaf-frozen ST value, never the X
 * observation), runs the single-read completion on the X word
 * with the accumulator effect, and delegates the READY commit
 * to the store-compute commit owner only after both legs
 * resolve. Reads issue before writes, so an X/Y alias
 * computes from the pre-write value. The second-leg flag
 * rides the free candidate_rmw_operation byte; no transaction
 * grows. No architectural field is written until pair
 * completion passes all candidate and live-state checks.
 * Does not own: opcode semantics, address arithmetic, endpoint
 * storage, timing, CPU lifecycle, observers, diagnostics,
 * allocation, I/O, or global state. Transaction storage is
 * borrowed for each call.
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

bool ti_c54x_memory_instruction_dual_store_compute_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction);

bool ti_c54x_memory_instruction_dual_store_compute_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_dual_store_compute_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const struct ti_c54x_bus_word16_endpoint *endpoint,
    struct ti_c54x_arch_state *state,
    struct ti_c54x_accumulator_state *accumulators,
    struct ti_c54x_t_register_state *t_register,
    struct ti_c54x_memory_instruction_result *result);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_dual_store_compute_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const struct ti_c54x_bus_word16_endpoint *endpoint);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_dual_store_compute_reset(
    ti_c54x_memory_instruction_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_STORE_COMPUTE_H */
