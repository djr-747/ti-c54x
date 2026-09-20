/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied PORT pair
 * candidates; see memory_instruction_port_candidate_validate.h.
 * Does not own: transaction lifecycle, endpoint I/O, architectural
 * mutation, effect completion, opcode dispatch, timing, observers,
 * diagnostics, allocation, or global state. Inputs are borrowed for
 * one call.
 */

#include "memory_instruction_port_candidate_validate.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/instruction/memory_candidate_absolute_data.h"
#include "sequencer/instruction/memory_candidate_direct.h"
#include "sequencer/instruction/memory_candidate_single_leg.h"
#include "sequencer/instruction/memory_instruction_port.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/memory/address_resolution.h"
#include "state/arch_state.h"

bool
ti_c54x_memory_instruction_port_candidate_shape_is_valid(
    const struct ti_c54x_memory_instruction_candidate *candidate)
{
    ti_c54x_op_memory_effect data_effect;

    if (candidate == NULL ||
        candidate->effect.payload.memory.address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16) {
        return false;
    }
    /* The DATA leg rides the pair snapshot, not the candidate's
     * generic address resolution: that slot shares union storage
     * with the snapshot. Re-derive the DATA spec from the frozen
     * arm and input, validate the DATA leg against it, and bind
     * the IO request to the arm. */
    data_effect = candidate->effect.payload.memory;
    if (!ti_c54x_memory_instruction_port_synthesize_data_spec(
            &candidate->effect.payload.memory.address_spec,
            &candidate->input, &data_effect.address_spec)) {
        return false;
    }
    /* The absolute-Smem marker (P3-S85) synthesizes an ABSOLUTE_DATA
     * leg, which validates through the absolute-data shape (MVDM/MVMD
     * and Smem-absolute precedent), not the single-leg checker. */
    if (data_effect.address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16) {
        if (!ti_c54x_memory_candidate_absolute_data_shape_is_valid(
                &candidate->port_address_snapshot.data_resolution,
                &data_effect)) {
            return false;
        }
    } else if (!ti_c54x_memory_candidate_single_leg_shape_is_valid(
                   &candidate->port_address_snapshot.data_resolution,
                   &data_effect)) {
        return false;
    }
    return ti_c54x_memory_instruction_port_io_request_matches_arm(
        &candidate->port_address_snapshot.io_request,
        &candidate->effect.payload.memory);
}

/* PORT absolute-Smem DATA-leg liveness (P3-S85). The absolute leg
 * observes no state (absolute-data precedent): re-prove the frozen
 * absolute echo against the marker's DATA address and pin the
 * sequential PC. */
bool
ti_c54x_memory_instruction_port_absolute_data_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state,
    const struct ti_c54x_memory_address_resolution *resolution)
{
    const ti_c54x_address_effect *address_effect;
    uint16_t address;

    if (candidate == NULL || state == NULL || resolution == NULL ||
        candidate->effect.payload.memory.address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16) {
        return false;
    }
    address = candidate->effect.payload.memory.address_spec.port
                  .reserved_half;
    address_effect = &resolution->address_result.effect;
    if (address_effect->source.kind != TI_C54X_ADDRESS_SOURCE_ABSOLUTE ||
        address_effect->state_policy !=
            TI_C54X_ADDRESS_STATE_POLICY_NONE ||
        address_effect->source.value.absolute.address != address ||
        resolution->request.space !=
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        resolution->request.address_word != (uint32_t)address) {
        return false;
    }
    return ti_c54x_memory_candidate_pc_matches_live(&candidate->pc_effect,
                                                  state);
}

/* PORT direct-DP DATA-leg liveness. The PORT arm carries no DP page
 * echo (the page rides the frozen input and shape rebinds it through
 * the re-derived DATA spec on every check), so liveness pins the
 * CPL0 policy plus live CPL/DP against the DATA leg's own page and
 * then the sequential PC. Page-0 sources cannot arise from PORT
 * synthesis and reject. */
bool
ti_c54x_memory_instruction_port_direct_data_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const ti_c54x_arch_state *state,
    const ti_c54x_memory_address_resolution *resolution)
{
    const ti_c54x_address_effect *address_effect;
    uint8_t cpl = UINT8_C(0);
    uint16_t dp = UINT16_C(0);

    if (candidate == NULL || state == NULL || resolution == NULL) {
        return false;
    }
    address_effect = &resolution->address_result.effect;
    if (address_effect->source.kind !=
        TI_C54X_ADDRESS_SOURCE_DIRECT_DP) {
        return false;
    }
    if (address_effect->state_policy !=
            TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0 ||
        ti_c54x_arch_state_read_cpl(state, &cpl) !=
            TI_C54X_ARCH_STATE_OK ||
        cpl != UINT8_C(0) ||
        ti_c54x_arch_state_read_dp(state, &dp) !=
            TI_C54X_ARCH_STATE_OK ||
        dp != address_effect->source.value.direct_dp.dp9) {
        return false;
    }
    return ti_c54x_memory_candidate_pc_matches_live(&candidate->pc_effect,
                                                    state);
}
