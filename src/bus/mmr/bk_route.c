/*
 * Module contract
 *
 * Owns: the pure BK memory-mapped request router against borrowed state.
 * Does not own: BK storage or knownness policy, bus transfers, endpoint
 * callbacks, arbitration, full MMR/device routing, opcode semantics, CPU
 * lifecycle, timing, or diagnostics.
 */

#include "bk_route.h"

#include <stddef.h>
#include <stdint.h>

static ti_c54x_bus_mmr_bk_result bk_result(
    ti_c54x_bus_mmr_bk_status status,
    uint16_t read_value_word)
{
    ti_c54x_bus_mmr_bk_result result = {status, read_value_word};

    return result;
}

ti_c54x_bus_mmr_bk_result ti_c54x_bus_mmr_bk_route(
    const ti_c54x_bus_word16_request *request,
    ti_c54x_arch_state *state)
{
    uint16_t bk_value = UINT16_C(0);

    if (request == NULL || state == NULL) {
        return bk_result(TI_C54X_BUS_MMR_BK_STATUS_INVALID_ARGUMENT,
                         UINT16_C(0));
    }
    if ((request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
         request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) ||
        (request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM &&
         request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
         request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_IO)) {
        return bk_result(TI_C54X_BUS_MMR_BK_STATUS_INVALID_ARGUMENT,
                         UINT16_C(0));
    }
    if (request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        request->address_word !=
            (uint32_t)TI_C54X_BUS_MMR_BK_DATA_ADDRESS_WORD) {
        return bk_result(TI_C54X_BUS_MMR_BK_STATUS_DECLINED, UINT16_C(0));
    }
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return ti_c54x_arch_state_write_bk(state,
                                           request->write_value_word) ==
                       TI_C54X_ARCH_STATE_OK
                   ? bk_result(TI_C54X_BUS_MMR_BK_STATUS_SERVICED,
                               UINT16_C(0))
                   : bk_result(TI_C54X_BUS_MMR_BK_STATUS_INVALID_ARGUMENT,
                               UINT16_C(0));
    }
    return ti_c54x_arch_state_read_bk(state, &bk_value) ==
                   TI_C54X_ARCH_STATE_OK
               ? bk_result(TI_C54X_BUS_MMR_BK_STATUS_SERVICED, bk_value)
               : bk_result(TI_C54X_BUS_MMR_BK_STATUS_UNKNOWN, UINT16_C(0));
}
