/* Module contract
 *
 * Owns: structural and live-state validation of one copied
 * absolute-DATA WORD16 resolution against its memory effect;
 * see memory_candidate_absolute_data.h.
 * Does not own: other address kinds, transaction lifecycle,
 * endpoint I/O, architectural mutation, effect completion,
 * opcode dispatch, timing, observers, diagnostics, allocation,
 * or callbacks.
 */

#include "memory_candidate_absolute_data.h"

#include <stddef.h>
#include <stdint.h>

#include "address/data_operand.h"
#include "bus/mmr/mmr_route.h"
#include "bus/request/word16_request.h"
#include "isa/ops/memory/memory_effect.h"
#include "sequencer/instruction/memory_candidate_direct.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/memory/address_resolution.h"
#include "state/arch_state.h"

static bool
absolute_echo_matches(const ti_c54x_address_effect *address_effect,
                      uint16_t address)
{
    if (address_effect->source.kind !=
        TI_C54X_ADDRESS_SOURCE_ABSOLUTE) {
        return false;
    }
    return address_effect->source.value.absolute.address == address;
}

static bool
sidecar_is_routable(const ti_c54x_op_memory_effect *effect)
{
    uint8_t sidecar_kind =
        effect->address_spec.absolute_data.mmr_sidecar_kind;
    uint8_t sidecar_number =
        effect->address_spec.absolute_data.mmr_sidecar_number;

    if (sidecar_kind ==
        (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_NONE) {
        return sidecar_number == UINT8_C(0);
    }
    /* Both STORE and LOAD legs name a DATA MMR the router must
     * serve; the kind/access pairing itself is proven by the
     * effect gate. */
    if (sidecar_kind !=
            (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_STORE_WITH_READ &&
        sidecar_kind !=
            (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_LOAD_WITH_WRITE) {
        return false;
    }
    return ti_c54x_bus_mmr_is_routed_address(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        (uint32_t)sidecar_number);
}

bool
ti_c54x_memory_candidate_absolute_data_shape_is_valid(
    const ti_c54x_memory_address_resolution *resolution,
    const ti_c54x_op_memory_effect *effect)
{
    const ti_c54x_address_effect *address_effect;
    uint16_t address;

    if (resolution == NULL || effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16 ||
        resolution->address_result.status != TI_C54X_ADDRESS_STATUS_OK ||
        resolution->request.space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        resolution->request.access != effect->access ||
        (effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
         effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) ||
        !ti_c54x_bus_word16_request_is_valid(&resolution->request)) {
        return false;
    }
    address = effect->address_spec.absolute_data.address;
    address_effect = &resolution->address_result.effect;
    return address_effect->state_policy ==
               TI_C54X_ADDRESS_STATE_POLICY_NONE &&
           absolute_echo_matches(address_effect, address) &&
           address_effect->operand.width ==
               TI_C54X_DATA_OPERAND_WIDTH_WORD16 &&
           address_effect->operand.first_word == address &&
           resolution->request.address_word == (uint32_t)address &&
           sidecar_is_routable(effect);
}

bool
ti_c54x_memory_candidate_absolute_data_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state)
{
    const ti_c54x_address_effect *address_effect;
    uint16_t address;

    if (candidate == NULL || state == NULL) {
        return false;
    }
    /* Absolute DATA observes no state: no CPL/DP read, so neither
     * the generic CMPT=0/ARP=0 gate nor any live source echo
     * applies. The NONE policy, the frozen absolute echo, and the
     * routable sidecar re-prove the shape; only the sequential PC
     * is live. */
    if (!ti_c54x_memory_candidate_absolute_data_shape_is_valid(
            &candidate->address_resolution,
            &candidate->effect.payload.memory)) {
        return false;
    }
    address =
        candidate->effect.payload.memory.address_spec.absolute_data
            .address;
    address_effect =
        &candidate->address_resolution.address_result.effect;
    return absolute_echo_matches(address_effect, address) &&
           sidecar_is_routable(
               &candidate->effect.payload.memory) &&
           ti_c54x_memory_candidate_pc_matches_live(
               &candidate->pc_effect, state);
}
