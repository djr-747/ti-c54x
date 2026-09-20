/*
 * Module contract
 *
 * Owns: pure validation of the PORT IO-leg arm and construction of
 * the copied IO-space request at the absolute port address. The
 * DATA-leg direction selects the inverse IO leg; WRITE templates
 * carry zero for the reducer to replace. The absolute-Smem marker
 * (mode 15) carries the DATA address in reserved_half. No state is
 * observed.
 * Does not own: DATA-leg resolution, request issue, endpoint
 * traffic, opcode semantics, CPU lifecycle, or timing.
 */

#include "port_access_plan.h"

#include <stddef.h>

#include "bus/request/word16_request.h"

static ti_c54x_port_access
invalid_plan(ti_c54x_port_access_status status)
{
    const ti_c54x_port_access plan = {status,
                                      {TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID,
                                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID,
                                       UINT32_C(0), UINT16_C(0)}};

    return plan;
}

ti_c54x_port_access ti_c54x_sequencer_plan_port_access(
    const ti_c54x_op_memory_address_spec *spec,
    ti_c54x_bus_word16_request_access data_access)
{
    ti_c54x_bus_word16_request_result built;

    if (spec == NULL) {
        return invalid_plan(TI_C54X_PORT_ACCESS_INVALID_ARGUMENT);
    }
    if (spec->kind != TI_C54X_OP_MEMORY_ADDRESS_PORT_WORD16) {
        return invalid_plan(TI_C54X_PORT_ACCESS_INVALID_SPEC);
    }
    if (data_access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
        data_access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return invalid_plan(TI_C54X_PORT_ACCESS_INVALID_SPEC);
    }
    if (spec->port.indirect > UINT8_C(1)) {
        return invalid_plan(TI_C54X_PORT_ACCESS_INVALID_SPEC);
    }
    if (spec->port.indirect == UINT8_C(0)) {
        if (spec->port.smem_mode != UINT8_C(0) ||
            spec->port.smem_arf != UINT8_C(0) ||
            spec->port.smem_dma7 > UINT8_C(0x7F)) {
            return invalid_plan(TI_C54X_PORT_ACCESS_INVALID_SPEC);
        }
    } else {
        /* Absolute Smem (P3-S85, TI SPRU172C §4 pp. 4-129–4-130):
         * the absolute marker (mode=15, arf=0, dma7=0) carries
         * the 16-bit absolute DATA address in reserved_half. */
        if (spec->port.smem_mode == UINT8_C(15)) {
            if (spec->port.smem_arf != UINT8_C(0) ||
                spec->port.smem_dma7 != UINT8_C(0)) {
                return invalid_plan(TI_C54X_PORT_ACCESS_INVALID_SPEC);
            }
        } else if (spec->port.smem_dma7 != UINT8_C(0) ||
                   spec->port.smem_arf > UINT8_C(7) ||
                   spec->port.smem_mode > UINT8_C(10)) {
            return invalid_plan(TI_C54X_PORT_ACCESS_INVALID_SPEC);
        }
    }
    if (spec->port.reserved_word0 != UINT32_C(0) ||
        spec->port.reserved_word1 != UINT32_C(0) ||
        spec->port.reserved_word2 != UINT32_C(0)) {
        return invalid_plan(TI_C54X_PORT_ACCESS_INVALID_SPEC);
    }
    if (spec->port.reserved_half != UINT16_C(0) &&
        !(spec->port.indirect == UINT8_C(1) &&
          spec->port.smem_mode == UINT8_C(15))) {
        return invalid_plan(TI_C54X_PORT_ACCESS_INVALID_SPEC);
    }
    if (data_access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        built = ti_c54x_bus_word16_request_read(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_IO, spec->port.port_address);
    } else {
        built = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_IO, spec->port.port_address,
            UINT16_C(0));
    }
    if (built.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return invalid_plan(TI_C54X_PORT_ACCESS_INVALID_SPEC);
    }
    {
        const ti_c54x_port_access plan = {TI_C54X_PORT_ACCESS_IO_REQUEST,
                                          built.request};

        return plan;
    }
}
