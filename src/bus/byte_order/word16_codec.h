#ifndef TI_C54X_BUS_BYTE_ORDER_WORD16_CODEC_H
#define TI_C54X_BUS_BYTE_ORDER_WORD16_CODEC_H

/*
 * Module contract
 *
 * Owns: the private, host-order-independent conversion of one architectural
 * WORD16 value to or from exactly two byte lanes under an explicit order.
 * Does not own: C54x or device byte-order claims, image or wire formats,
 * shared-memory lanes, bus requests, transactions, CPU state, timing,
 * arbitration, diagnostics, or public ABI.
 *
 * Encoding consumes a copied scalar and returns value-owned bytes. Decoding
 * borrows exactly two readable bytes until return and retains no pointer.
 * Invalid results carry zero in every named payload field.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_bus_word16_codec_order {
    TI_C54X_BUS_WORD16_CODEC_ORDER_INVALID = 0,
    TI_C54X_BUS_WORD16_CODEC_ORDER_BIG = 1,
    TI_C54X_BUS_WORD16_CODEC_ORDER_LITTLE = 2
} ti_c54x_bus_word16_codec_order;

typedef enum ti_c54x_bus_word16_codec_status {
    TI_C54X_BUS_WORD16_CODEC_STATUS_OK = 0,
    TI_C54X_BUS_WORD16_CODEC_STATUS_INVALID_ARGUMENT = 1,
    TI_C54X_BUS_WORD16_CODEC_STATUS_INVALID_ORDER = 2
} ti_c54x_bus_word16_codec_status;

typedef struct ti_c54x_bus_word16_codec_encode_result {
    ti_c54x_bus_word16_codec_status status;
    uint8_t bytes[2];
} ti_c54x_bus_word16_codec_encode_result;

typedef struct ti_c54x_bus_word16_codec_decode_result {
    ti_c54x_bus_word16_codec_status status;
    uint16_t value_word;
} ti_c54x_bus_word16_codec_decode_result;

ti_c54x_bus_word16_codec_encode_result ti_c54x_bus_word16_codec_encode(
    uint16_t value_word,
    ti_c54x_bus_word16_codec_order order);

ti_c54x_bus_word16_codec_decode_result ti_c54x_bus_word16_codec_decode(
    const uint8_t bytes[2],
    ti_c54x_bus_word16_codec_order order);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_BUS_BYTE_ORDER_WORD16_CODEC_H */
