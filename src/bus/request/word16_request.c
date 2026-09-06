/*
 * Module contract
 *
 * Owns: checked construction and validation of one value-owned architectural
 * WORD16 bus request.
 * Does not own: callbacks, bus issue or completion, byte order, memory,
 * sequencer phases, CPU state, opcode effects, timing, arbitration, or
 * diagnostics.
 */

#include "word16_request.h"

#include <stddef.h>
#include <stdint.h>

static ti_c54x_bus_word16_request_result
ti_c54x_bus_word16_request_invalid(ti_c54x_bus_word16_request_status status)
{
    ti_c54x_bus_word16_request_result result = {0};

    result.status = status;
    return result;
}

static bool ti_c54x_bus_word16_request_space_is_valid(
    ti_c54x_bus_word16_request_space space)
{
    return space == TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM ||
           space == TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
           space == TI_C54X_BUS_WORD16_REQUEST_SPACE_IO;
}

static bool ti_c54x_bus_word16_request_address_is_valid(
    ti_c54x_bus_word16_request_space space,
    uint32_t address_word)
{
    return space == TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM ||
           address_word <= (uint32_t)UINT16_MAX;
}

static ti_c54x_bus_word16_request_result ti_c54x_bus_word16_request_build(
    ti_c54x_bus_word16_request_space space,
    ti_c54x_bus_word16_request_access access,
    uint32_t address_word,
    uint16_t write_value_word)
{
    ti_c54x_bus_word16_request_result result = {0};

    if (!ti_c54x_bus_word16_request_space_is_valid(space)) {
        return ti_c54x_bus_word16_request_invalid(
            TI_C54X_BUS_WORD16_REQUEST_STATUS_INVALID_SPACE);
    }
    if (!ti_c54x_bus_word16_request_address_is_valid(space, address_word)) {
        return ti_c54x_bus_word16_request_invalid(
            TI_C54X_BUS_WORD16_REQUEST_STATUS_INVALID_ADDRESS);
    }

    result.status = TI_C54X_BUS_WORD16_REQUEST_STATUS_OK;
    result.request.space = space;
    result.request.access = access;
    result.request.address_word = address_word;
    result.request.write_value_word = write_value_word;
    return result;
}

ti_c54x_bus_word16_request_result ti_c54x_bus_word16_request_read(
    ti_c54x_bus_word16_request_space space,
    uint32_t address_word)
{
    return ti_c54x_bus_word16_request_build(
        space, TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, address_word,
        UINT16_C(0));
}

ti_c54x_bus_word16_request_result ti_c54x_bus_word16_request_write(
    ti_c54x_bus_word16_request_space space,
    uint32_t address_word,
    uint16_t write_value_word)
{
    return ti_c54x_bus_word16_request_build(
        space, TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, address_word,
        write_value_word);
}

bool ti_c54x_bus_word16_request_is_valid(
    const ti_c54x_bus_word16_request *request)
{
    if (request == NULL ||
        !ti_c54x_bus_word16_request_space_is_valid(request->space)) {
        return false;
    }
    if (request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
        request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return false;
    }
    if (!ti_c54x_bus_word16_request_address_is_valid(request->space,
                                                     request->address_word)) {
        return false;
    }
    return request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
           request->write_value_word == UINT16_C(0);
}
