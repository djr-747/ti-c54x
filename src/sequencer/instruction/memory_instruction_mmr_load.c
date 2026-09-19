/*
 * Module contract
 *
 * Owns: the read-phase MMR load snapshot (MVMD); see
 * memory_instruction_mmr_load.h.
 * Does not own: carrier validity, address resolution, candidate
 * shape or liveness, the issue orchestrator itself, CPU
 * publication, endpoint callbacks, timing, observers,
 * diagnostics, allocation, or callbacks.
 */

#include "memory_instruction_mmr_load.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bus/mmr/mmr_route.h"
#include "bus/request/word16_request.h"
#include "isa/ops/memory/memory_effect.h"
#include "sequencer/memory/mmr_access_plan.h"

bool
ti_c54x_memory_instruction_snapshot_mmr_load(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register)
{
    const ti_c54x_op_memory_effect *memory;
    uint32_t mmr_number;
    ti_c54x_sequencer_mmr_access load_plan;
    ti_c54x_bus_word16_request_result load_built;
    ti_c54x_bus_mmr_route_result load_result;

    if (transaction == NULL) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    if (transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY ||
        memory->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16 ||
        memory->address_spec.absolute_data.mmr_sidecar_kind !=
            (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_LOAD_WITH_WRITE) {
        return true;
    }
    /* The gate pairs LOAD legs with WRITE access and a zero
     * staged value; re-check defensively since this patches the
     * request the endpoint will issue. */
    if (memory->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ||
        memory->value_word != UINT16_C(0)) {
        return false;
    }
    mmr_number = (uint32_t)memory->address_spec.absolute_data
                     .mmr_sidecar_number;
    load_plan = ti_c54x_sequencer_plan_mmr_access(
        state, accumulators, t_register,
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, mmr_number,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT16_C(0));
    if (load_plan != TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED) {
        return false;
    }
    load_built = ti_c54x_bus_word16_request_read(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, mmr_number);
    if (load_built.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return false;
    }
    load_result = ti_c54x_bus_mmr_route(&load_built.request, state,
                                        accumulators, t_register);
    if (load_result.status != TI_C54X_BUS_MMR_STATUS_SERVICED) {
        return false;
    }
    transaction->candidate.address_resolution.request.write_value_word =
        load_result.read_value_word;
    return true;
}
