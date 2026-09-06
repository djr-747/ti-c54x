/*
 * Module contract
 *
 * Owns: checked, explicit BIG/LITTLE conversion of one WORD16 value and two
 * byte lanes.
 * Does not own: host byte order, device or image policy, shared memory, bus
 * requests, transactions, CPU state, timing, arbitration, or diagnostics.
 */

#include "word16_codec.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool ti_c54x_bus_word16_codec_order_is_valid(
    ti_c54x_bus_word16_codec_order order)
{
    return order == TI_C54X_BUS_WORD16_CODEC_ORDER_BIG ||
           order == TI_C54X_BUS_WORD16_CODEC_ORDER_LITTLE;
}

static ti_c54x_bus_word16_codec_encode_result
ti_c54x_bus_word16_codec_encode_invalid(
    ti_c54x_bus_word16_codec_status status)
{
    ti_c54x_bus_word16_codec_encode_result result = {0};

    result.status = status;
    return result;
}

static ti_c54x_bus_word16_codec_decode_result
ti_c54x_bus_word16_codec_decode_invalid(
    ti_c54x_bus_word16_codec_status status)
{
    ti_c54x_bus_word16_codec_decode_result result = {0};

    result.status = status;
    return result;
}

ti_c54x_bus_word16_codec_encode_result ti_c54x_bus_word16_codec_encode(
    uint16_t value_word,
    ti_c54x_bus_word16_codec_order order)
{
    ti_c54x_bus_word16_codec_encode_result result = {0};

    if (!ti_c54x_bus_word16_codec_order_is_valid(order)) {
        return ti_c54x_bus_word16_codec_encode_invalid(
            TI_C54X_BUS_WORD16_CODEC_STATUS_INVALID_ORDER);
    }

    result.status = TI_C54X_BUS_WORD16_CODEC_STATUS_OK;
    if (order == TI_C54X_BUS_WORD16_CODEC_ORDER_BIG) {
        result.bytes[0] = (uint8_t)((uint32_t)value_word >> 8U);
        result.bytes[1] = (uint8_t)value_word;
    } else {
        result.bytes[0] = (uint8_t)value_word;
        result.bytes[1] = (uint8_t)((uint32_t)value_word >> 8U);
    }
    return result;
}

ti_c54x_bus_word16_codec_decode_result ti_c54x_bus_word16_codec_decode(
    const uint8_t bytes[2],
    ti_c54x_bus_word16_codec_order order)
{
    ti_c54x_bus_word16_codec_decode_result result = {0};
    uint32_t high_lane;
    uint32_t low_lane;

    if (bytes == NULL) {
        return ti_c54x_bus_word16_codec_decode_invalid(
            TI_C54X_BUS_WORD16_CODEC_STATUS_INVALID_ARGUMENT);
    }
    if (!ti_c54x_bus_word16_codec_order_is_valid(order)) {
        return ti_c54x_bus_word16_codec_decode_invalid(
            TI_C54X_BUS_WORD16_CODEC_STATUS_INVALID_ORDER);
    }

    if (order == TI_C54X_BUS_WORD16_CODEC_ORDER_BIG) {
        high_lane = (uint32_t)bytes[0];
        low_lane = (uint32_t)bytes[1];
    } else {
        high_lane = (uint32_t)bytes[1];
        low_lane = (uint32_t)bytes[0];
    }

    result.status = TI_C54X_BUS_WORD16_CODEC_STATUS_OK;
    result.value_word = (uint16_t)((high_lane << 8U) | low_lane);
    return result;
}
