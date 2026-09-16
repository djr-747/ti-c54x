/*
 * Module contract
 *
 * Owns: the pure MMR access plan against borrowed state. READ
 * probes are side-effect-free router dry runs; WRITE plans never
 * touch state. Does not own: register storage, request issue,
 * endpoint traffic, opcode semantics, CPU lifecycle, or timing.
 */

#include "mmr_access_plan.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/mmr/mmr_route.h"

ti_c54x_sequencer_mmr_access ti_c54x_sequencer_plan_mmr_access(
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_bus_word16_request_space space, uint32_t address_word,
    ti_c54x_bus_word16_request_access access, uint16_t write_value_word)
{
    uint16_t address;
    ti_c54x_bus_word16_request probe;
    ti_c54x_bus_mmr_route_result probed;

    if (state == NULL || accumulators == NULL || t_register == NULL) {
        return TI_C54X_SEQUENCER_MMR_ACCESS_INVALID_ARGUMENT;
    }
    if ((access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
         access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) ||
        (space != TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM &&
         space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
         space != TI_C54X_BUS_WORD16_REQUEST_SPACE_IO)) {
        return TI_C54X_SEQUENCER_MMR_ACCESS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_bus_mmr_is_routed_address(space, address_word)) {
        return TI_C54X_SEQUENCER_MMR_ACCESS_ENDPOINT;
    }
    address = (uint16_t)address_word;
    if (access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        /* Guard words reject above 0xFF at the owner; every other
         * MMR write is unconditionally serviceable. */
        if ((address == TI_C54X_BUS_MMR_ADDRESS_AG ||
             address == TI_C54X_BUS_MMR_ADDRESS_BG) &&
            write_value_word > UINT16_C(0x00FF)) {
            return TI_C54X_SEQUENCER_MMR_ACCESS_INVALID_ARGUMENT;
        }
        return TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED;
    }
    probe.space = space;
    probe.access = access;
    probe.address_word = address_word;
    probe.write_value_word = UINT16_C(0);
    probed = ti_c54x_bus_mmr_route(&probe, state, accumulators,
                                   t_register);
    switch (probed.status) {
    case TI_C54X_BUS_MMR_STATUS_SERVICED:
        return TI_C54X_SEQUENCER_MMR_ACCESS_ROUTED;
    case TI_C54X_BUS_MMR_STATUS_UNKNOWN:
        return TI_C54X_SEQUENCER_MMR_ACCESS_UNKNOWN;
    case TI_C54X_BUS_MMR_STATUS_DECLINED:
        return TI_C54X_SEQUENCER_MMR_ACCESS_ENDPOINT;
    case TI_C54X_BUS_MMR_STATUS_INVALID_ARGUMENT:
    default:
        return TI_C54X_SEQUENCER_MMR_ACCESS_INVALID_ARGUMENT;
    }
}
