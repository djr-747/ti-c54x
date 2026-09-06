/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proof of the private value-owned
 * WORD16 BIG/LITTLE byte-lane codec.
 * Does not own: TI or device byte-order claims, image or wire formats, shared
 * memory lanes, bus requests, transactions, CPU state, timing, diagnostics,
 * or test discovery.
 */

#include "../../../../src/bus/byte_order/word16_codec.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

static void check_zero_encode_result(
    const ti_c54x_bus_word16_codec_encode_result *result,
    ti_c54x_bus_word16_codec_status status)
{
    TI_C54X_TEST_CHECK(result->status == status);
    TI_C54X_TEST_CHECK(result->bytes[0] == UINT8_C(0));
    TI_C54X_TEST_CHECK(result->bytes[1] == UINT8_C(0));
}

static void check_zero_decode_result(
    const ti_c54x_bus_word16_codec_decode_result *result,
    ti_c54x_bus_word16_codec_status status)
{
    TI_C54X_TEST_CHECK(result->status == status);
    TI_C54X_TEST_CHECK(result->value_word == UINT16_C(0));
}

static void test_round_trip_all_words(
    ti_c54x_bus_word16_codec_order order)
{
    uint32_t value;

    for (value = UINT32_C(0); value <= (uint32_t)UINT16_MAX; ++value) {
        const uint16_t word = (uint16_t)value;
        const ti_c54x_bus_word16_codec_encode_result encoded =
            ti_c54x_bus_word16_codec_encode(word, order);
        const ti_c54x_bus_word16_codec_decode_result decoded =
            ti_c54x_bus_word16_codec_decode(encoded.bytes, order);

        TI_C54X_TEST_CHECK(encoded.status ==
                           TI_C54X_BUS_WORD16_CODEC_STATUS_OK);
        TI_C54X_TEST_CHECK(decoded.status ==
                           TI_C54X_BUS_WORD16_CODEC_STATUS_OK);
        TI_C54X_TEST_CHECK_U64(decoded.value_word, word);
    }
}

static void test_exact_lane_vectors(void)
{
    const uint8_t big_bytes[2] = {UINT8_C(0x12), UINT8_C(0x34)};
    const uint8_t little_bytes[2] = {UINT8_C(0x34), UINT8_C(0x12)};
    const ti_c54x_bus_word16_codec_encode_result big =
        ti_c54x_bus_word16_codec_encode(
            UINT16_C(0x1234), TI_C54X_BUS_WORD16_CODEC_ORDER_BIG);
    const ti_c54x_bus_word16_codec_encode_result little =
        ti_c54x_bus_word16_codec_encode(
            UINT16_C(0x1234), TI_C54X_BUS_WORD16_CODEC_ORDER_LITTLE);
    const ti_c54x_bus_word16_codec_encode_result zero =
        ti_c54x_bus_word16_codec_encode(
            UINT16_C(0x0000), TI_C54X_BUS_WORD16_CODEC_ORDER_BIG);
    const ti_c54x_bus_word16_codec_encode_result maximum =
        ti_c54x_bus_word16_codec_encode(
            UINT16_C(0xffff), TI_C54X_BUS_WORD16_CODEC_ORDER_LITTLE);
    const ti_c54x_bus_word16_codec_decode_result decoded_big =
        ti_c54x_bus_word16_codec_decode(
            big_bytes, TI_C54X_BUS_WORD16_CODEC_ORDER_BIG);
    const ti_c54x_bus_word16_codec_decode_result decoded_little =
        ti_c54x_bus_word16_codec_decode(
            little_bytes, TI_C54X_BUS_WORD16_CODEC_ORDER_LITTLE);

    TI_C54X_TEST_CHECK(big.status == TI_C54X_BUS_WORD16_CODEC_STATUS_OK);
    TI_C54X_TEST_CHECK(big.bytes[0] == UINT8_C(0x12));
    TI_C54X_TEST_CHECK(big.bytes[1] == UINT8_C(0x34));
    TI_C54X_TEST_CHECK(little.status == TI_C54X_BUS_WORD16_CODEC_STATUS_OK);
    TI_C54X_TEST_CHECK(little.bytes[0] == UINT8_C(0x34));
    TI_C54X_TEST_CHECK(little.bytes[1] == UINT8_C(0x12));
    TI_C54X_TEST_CHECK(zero.status == TI_C54X_BUS_WORD16_CODEC_STATUS_OK);
    TI_C54X_TEST_CHECK(zero.bytes[0] == UINT8_C(0x00));
    TI_C54X_TEST_CHECK(zero.bytes[1] == UINT8_C(0x00));
    TI_C54X_TEST_CHECK(maximum.status ==
                       TI_C54X_BUS_WORD16_CODEC_STATUS_OK);
    TI_C54X_TEST_CHECK(maximum.bytes[0] == UINT8_C(0xff));
    TI_C54X_TEST_CHECK(maximum.bytes[1] == UINT8_C(0xff));
    TI_C54X_TEST_CHECK(decoded_big.status ==
                       TI_C54X_BUS_WORD16_CODEC_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(decoded_big.value_word, UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(decoded_little.status ==
                       TI_C54X_BUS_WORD16_CODEC_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(decoded_little.value_word, UINT16_C(0x1234));
}

static void test_invalid_orders_zero_payloads(void)
{
    const ti_c54x_bus_word16_codec_order invalid_orders[] = {
        TI_C54X_BUS_WORD16_CODEC_ORDER_INVALID,
        (ti_c54x_bus_word16_codec_order)UINT8_C(0x7f)};
    const uint8_t bytes[] = {UINT8_C(0x12), UINT8_C(0x34)};
    size_t index;

    for (index = 0U;
         index < sizeof(invalid_orders) / sizeof(invalid_orders[0]);
         ++index) {
        const ti_c54x_bus_word16_codec_encode_result encoded =
            ti_c54x_bus_word16_codec_encode(UINT16_C(0xbeef),
                                             invalid_orders[index]);
        const ti_c54x_bus_word16_codec_decode_result decoded =
            ti_c54x_bus_word16_codec_decode(bytes, invalid_orders[index]);

        check_zero_encode_result(
            &encoded, TI_C54X_BUS_WORD16_CODEC_STATUS_INVALID_ORDER);
        check_zero_decode_result(
            &decoded, TI_C54X_BUS_WORD16_CODEC_STATUS_INVALID_ORDER);
    }
}

static void test_decode_null_precedes_order_validation(void)
{
    const ti_c54x_bus_word16_codec_order orders[] = {
        TI_C54X_BUS_WORD16_CODEC_ORDER_BIG,
        TI_C54X_BUS_WORD16_CODEC_ORDER_INVALID,
        (ti_c54x_bus_word16_codec_order)UINT8_C(0x7f)};
    size_t index;

    for (index = 0U; index < sizeof(orders) / sizeof(orders[0]); ++index) {
        const ti_c54x_bus_word16_codec_decode_result result =
            ti_c54x_bus_word16_codec_decode(NULL, orders[index]);

        check_zero_decode_result(
            &result, TI_C54X_BUS_WORD16_CODEC_STATUS_INVALID_ARGUMENT);
    }
}

static void test_decode_does_not_mutate_borrowed_lanes(void)
{
    const uint8_t original[] = {UINT8_C(0xa5), UINT8_C(0x5a)};
    const ti_c54x_bus_word16_codec_order orders[] = {
        TI_C54X_BUS_WORD16_CODEC_ORDER_BIG,
        TI_C54X_BUS_WORD16_CODEC_ORDER_LITTLE};
    const uint16_t expected_words[] = {UINT16_C(0xa55a), UINT16_C(0x5aa5)};
    size_t index;

    for (index = 0U; index < sizeof(orders) / sizeof(orders[0]); ++index) {
        uint8_t bytes[2] = {original[0], original[1]};
        const ti_c54x_bus_word16_codec_decode_result result =
            ti_c54x_bus_word16_codec_decode(bytes, orders[index]);

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_BUS_WORD16_CODEC_STATUS_OK);
        TI_C54X_TEST_CHECK_U64(result.value_word, expected_words[index]);
        TI_C54X_TEST_CHECK(bytes[0] == original[0]);
        TI_C54X_TEST_CHECK(bytes[1] == original[1]);
    }
}

int main(void)
{
    test_round_trip_all_words(TI_C54X_BUS_WORD16_CODEC_ORDER_BIG);
    test_round_trip_all_words(TI_C54X_BUS_WORD16_CODEC_ORDER_LITTLE);
    test_exact_lane_vectors();
    test_invalid_orders_zero_payloads();
    test_decode_null_precedes_order_validation();
    test_decode_does_not_mutate_borrowed_lanes();
    return 0;
}
