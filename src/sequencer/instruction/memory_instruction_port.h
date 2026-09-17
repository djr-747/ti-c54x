#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_PORT_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_PORT_H

/*
 * Module contract
 *
 * Owns: the root PORT I/O pair seam, including IO-leg planning,
 * DATA-leg synthesis through the existing single-leg resolver,
 * deferred two-request issue with observation handoff, atomic
 * address-commit publication, and terminal cancellation/reset.
 * The DATA-leg direction rides the effect access with the IO leg
 * always inverse (DATA WRITE reads IO for PORTR; DATA READ writes
 * IO for PORTW with the observed word replacing the zero template).
 * Does not own: opcode decode or semantics, address arithmetic, CPU
 * lifecycle, endpoint storage, timing, observers, diagnostics,
 * allocation, I/O, or global state. All candidate and architectural
 * inputs are borrowed for one call; accepted values remain in
 * caller-owned transaction storage.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "isa/ops/memory/memory_effect.h"
#include "memory_instruction_transaction.h"
#include "sequencer/memory/port_access_plan.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ti_c54x_memory_instruction_port_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction);

/* Re-derive the DATA-leg spec from the frozen arm and input.
 * Deterministic field moves only, never a replan: the shape
 * validator rechecks the stored DATA resolution against it, and
 * liveness still pins live state through the generic path. */
bool ti_c54x_memory_instruction_port_synthesize_data_spec(
    const ti_c54x_op_memory_address_spec *arm,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_memory_address_spec *data_spec);

/* Bind a planned IO-leg request to the arm: IO space, absolute
 * port address, inverse DATA-leg direction, zero template value. */
bool ti_c54x_memory_instruction_port_io_request_matches_arm(
    const ti_c54x_bus_word16_request *io_request,
    const ti_c54x_op_memory_effect *memory);

/* Validate the arm, plan the IO leg, resolve the DATA leg through
 * the single-leg resolver, and freeze the snapshot. */
bool ti_c54x_memory_instruction_port_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_port_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_port_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_port_reset(
    ti_c54x_memory_instruction_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_PORT_H */
