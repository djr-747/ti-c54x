#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_SMEM_ABSOLUTE_PAIR_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_SMEM_ABSOLUTE_PAIR_H

/*
 * Module contract
 *
 * Owns: root orchestration for one Smem-to-absolute DATA pair
 * (MVDK): Smem DATA-leg synthesis through the existing
 * single-leg resolver, absolute dmad-leg planning through the
 * absolute resolver (both legs frozen in the pair snapshot),
 * deferred two-request issue with per-leg MMR-route-or-endpoint
 * service and observation handoff, atomic address-commit
 * publication, and terminal cancellation/reset. MVDK reads Smem
 * first and writes the observed word to dmad; no architectural
 * field is written until both legs and all candidate and
 * live-state checks pass. MVKD reuses this root with access
 * READ (S61).
 * Does not own: opcode semantics, address arithmetic, endpoint
 * storage, timing, CPU lifecycle, observers, diagnostics,
 * allocation, I/O, or global state. Transaction storage is
 * borrowed for each call.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "isa/ops/memory/memory_effect.h"
#include "memory_instruction_smem_absolute_pair_candidate_validate.h"
#include "memory_instruction_transaction.h"

#include "state/accumulator/accumulator.h"
#include "state/arch_state.h"
#include "state/temporary/t_register.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ti_c54x_memory_instruction_smem_absolute_pair_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction);

bool ti_c54x_memory_instruction_smem_absolute_pair_synthesize_data_spec(
    const ti_c54x_op_memory_address_spec *arm,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_memory_address_spec *data_spec);

bool ti_c54x_memory_instruction_smem_absolute_pair_synthesize_dmad_spec(
    const ti_c54x_op_memory_address_spec *arm,
    ti_c54x_op_memory_address_spec *dmad_spec);

bool ti_c54x_memory_instruction_smem_absolute_pair_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_arch_state *state);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_smem_absolute_pair_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_smem_absolute_pair_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint);

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_smem_absolute_pair_reset(
    ti_c54x_memory_instruction_transaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_SMEM_ABSOLUTE_PAIR_H */
