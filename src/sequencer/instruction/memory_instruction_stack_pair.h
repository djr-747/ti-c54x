#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_STACK_PAIR_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_STACK_PAIR_H

/*
 * Module contract
 *
 * Owns: the root stack-pair seam for PSHD/POPD direct pairs:
 * Smem DATA-leg synthesis through the existing single-leg
 * resolver, TOS stack-leg resolution through the generic stack
 * push/pop resolver (both legs frozen in the pair snapshot; the
 * SP commit rides the routed architecture commit), deferred
 * two-request issue with observation handoff (MMR-routed legs
 * serve synchronously through the S1 seam with no endpoint
 * traffic), and terminal cancellation/reset. PSHD reads Smem
 * first and writes the
 * observed word to TOS (effect access WRITE); POPD reads TOS
 * first and writes the observed word to Smem (effect access
 * READ). No architectural field is written until both legs and
 * all candidate and live-state checks pass.
 * Does not own: opcode decode or semantics, address arithmetic,
 * CPU lifecycle, endpoint storage, timing, observers,
 * diagnostics, allocation, or global state. All candidate and
 * architectural inputs are borrowed for one call; accepted
 * values remain in caller-owned transaction storage.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "isa/ops/memory/memory_effect.h"
#include "memory_instruction_stack_pair_candidate_validate.h"
#include "memory_instruction_transaction.h"

#include "state/accumulator/accumulator.h"
#include "state/temporary/t_register.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ti_c54x_memory_instruction_stack_pair_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction);

/* Re-derive a leg spec from the frozen arm and input.
 * Deterministic field moves only, never a replan. Direct-only:
 * indirect arms reject. */
bool ti_c54x_memory_instruction_stack_pair_synthesize_data_spec(
    const ti_c54x_op_memory_address_spec *arm,
    ti_c54x_op_memory_address_spec *data_spec);

bool ti_c54x_memory_instruction_stack_pair_synthesize_stack_spec(
    const ti_c54x_op_memory_address_spec *arm,
    ti_c54x_op_memory_address_kind *stack_kind);

/* Validate the arm, resolve both legs, and freeze the snapshot:
 * the TOS stack-leg and Smem DATA-leg resolutions both land in
 * the pair snapshot (the generic slot aliases that storage). */
bool ti_c54x_memory_instruction_stack_pair_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_stack_pair_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_stack_pair_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint);

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_stack_pair_reset(
    ti_c54x_memory_instruction_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_STACK_PAIR_H */
