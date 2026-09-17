/*
 * Module contract
 *
 * Owns: RED proofs for the S3 multiply-accumulate seam core, including the
 * three TI SPRU172C MAC vectors (pp. 4-81--4-83), the MPY store vectors
 * (pp. 4-101--4-102), R rounding, OVM saturation with OVdst disposition,
 * MAS subtraction, STORE-accumulator independence, the unsigned-operands
 * select (MPYU vector p. 4-106), and transactional
 * unknown/invalid stops. Does not own: catalog rows, operand layouts,
 * addressing, T/accumulator/status mutation, CPU sequencing, timing,
 * observers, diagnostics, or the production seam core.
 */

#include "../../../../../src/isa/ops/multiply/op_multiply.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

#define U40_MASK UINT64_C(0xFFFFFFFFFF)

static ti_c54x_multiply_operands base_operands(void)
{
    const ti_c54x_multiply_operands operands = {
        UINT16_C(0x0400),
        true,
        UINT16_C(0x1234),
        true,
        UINT64_C(0x1000),
        true,
        UINT8_C(0),
        true,
        UINT8_C(0),
        true,
        false,
        false,
        TI_C54X_MULTIPLY_ADD,
    };
    return operands;
}

static void test_ti_mac_example_vectors(void)
{
    ti_c54x_multiply_operands operands = base_operands();
    ti_c54x_multiply_result result = {0U, false};

    /* SPRU172C MAC Example 1: 0x1234 * 0x0400 + 0x1000, FRCT = 0. */
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x48E000));
    TI_C54X_TEST_CHECK(!result.overflow);

    /* SPRU172C MAC Example 2 shape: FRCT = 1 shifts the product left by
     * one before adding src: 0x0345 * 0x0400 * 2 + 0x1000. */
    operands.multiplicand_raw16 = UINT16_C(0x0345);
    operands.frct = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x1A3800));
    TI_C54X_TEST_CHECK(!result.overflow);

    /* SPRU172C MAC Example 3 shape: 0x5678 * 0x1234 + 0x1000. */
    operands.t_raw16 = UINT16_C(0x1234);
    operands.multiplicand_raw16 = UINT16_C(0x5678);
    operands.frct = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x6261060));
    TI_C54X_TEST_CHECK(!result.overflow);
}

static void test_mpy_store_vectors(void)
{
    ti_c54x_multiply_operands operands = base_operands();
    ti_c54x_multiply_result result = {0U, false};

    operands.select = TI_C54X_MULTIPLY_STORE;

    /* SPRU172C MPY Example 1 shape: 6 * 7, FRCT = 1 stores 84. */
    operands.t_raw16 = UINT16_C(6);
    operands.multiplicand_raw16 = UINT16_C(7);
    operands.frct = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(84));
    TI_C54X_TEST_CHECK(!result.overflow);

    /* SPRU172C MPY Example 3 shape: 0x2000 * 0xFFFE sign-extends. */
    operands.t_raw16 = UINT16_C(0x2000);
    operands.multiplicand_raw16 = UINT16_C(0xFFFE);
    operands.frct = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 ==
                       UINT64_C(0xFFFFFFC000));
    TI_C54X_TEST_CHECK(!result.overflow);

    /* SPRU172C MPY Example 4 shape: R rounds 0x6260060 to 0x6260000. */
    operands.t_raw16 = UINT16_C(0x1234);
    operands.multiplicand_raw16 = UINT16_C(0x5678);
    operands.rounding = true;
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x6260000));
    TI_C54X_TEST_CHECK(!result.overflow);
}

static void test_rounding_pair(void)
{
    ti_c54x_multiply_operands operands = base_operands();
    ti_c54x_multiply_result result = {0U, false};

    operands.select = TI_C54X_MULTIPLY_STORE;
    operands.t_raw16 = UINT16_C(0x4000);
    operands.multiplicand_raw16 = UINT16_C(2);
    operands.accumulator_raw40 = UINT64_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x8000));

    operands.rounding = true;
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x10000));
    TI_C54X_TEST_CHECK(!result.overflow);
}

static void test_mas_subtract(void)
{
    ti_c54x_multiply_operands operands = base_operands();
    ti_c54x_multiply_result result = {0U, false};

    operands.select = TI_C54X_MULTIPLY_SUBTRACT;
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 ==
                       UINT64_C(0xFFFFB74000));
    TI_C54X_TEST_CHECK(!result.overflow);
}

static void test_ovm_saturation(void)
{
    ti_c54x_multiply_operands operands = base_operands();
    ti_c54x_multiply_result result = {0U, false};

    /* Positive 32-bit overflow: 0x7FFFFFFF + 1. */
    operands.accumulator_raw40 = UINT64_C(0x7FFFFFFF);
    operands.t_raw16 = UINT16_C(1);
    operands.multiplicand_raw16 = UINT16_C(1);
    operands.ovm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x7FFFFFFF));
    TI_C54X_TEST_CHECK(result.overflow);

    /* OVM = 0 wraps the same overflow in the 40-bit accumulator. */
    operands.ovm = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x80000000));
    TI_C54X_TEST_CHECK(result.overflow);

    /* Negative 32-bit overflow: -2^31 + -1 saturates to FF 8000 0000. */
    operands.accumulator_raw40 = UINT64_C(0xFF80000000);
    operands.t_raw16 = UINT16_C(0xFFFF);
    operands.ovm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 ==
                       UINT64_C(0xFF80000000));
    TI_C54X_TEST_CHECK(result.overflow);

    /* OVM = 0 preserves the wrapped 40-bit negative result. */
    operands.ovm = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 ==
                       UINT64_C(0xFF7FFFFFFF));
    TI_C54X_TEST_CHECK(result.overflow);
}

static void test_store_ignores_accumulator(void)
{
    ti_c54x_multiply_operands operands = base_operands();
    ti_c54x_multiply_result result = {0U, false};

    operands.select = TI_C54X_MULTIPLY_STORE;
    operands.accumulator_known = false;
    operands.accumulator_raw40 = UINT64_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x48D000));
    TI_C54X_TEST_CHECK(!result.overflow);
}

static void test_unknown_stops(void)
{
    ti_c54x_multiply_operands operands = base_operands();
    ti_c54x_multiply_result result = {U40_MASK, true};

    operands.t_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_UNKNOWN);
    operands = base_operands();

    operands.multiplicand_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_UNKNOWN);
    operands = base_operands();

    operands.accumulator_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_UNKNOWN);
    operands = base_operands();

    operands.frct_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_UNKNOWN);
    operands = base_operands();

    operands.ovm_known = false;
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_UNKNOWN);

    /* No result is staged on an unknown stop. */
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == U40_MASK);
}

static void test_invalid_rejects(void)
{
    ti_c54x_multiply_operands operands = base_operands();
    ti_c54x_multiply_result result = {0U, false};

    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(NULL, &result) ==
                       TI_C54X_MULTIPLY_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, NULL) ==
                       TI_C54X_MULTIPLY_INVALID_ARGUMENT);

    operands.select = TI_C54X_MULTIPLY_COUNT;
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_INVALID_ARGUMENT);
    operands = base_operands();

    operands.frct = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_INVALID_ARGUMENT);
    operands = base_operands();

    operands.ovm = UINT8_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_INVALID_ARGUMENT);
    operands = base_operands();

    operands.accumulator_raw40 = U40_MASK + UINT64_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_INVALID_ARGUMENT);
}

static void test_unsigned_operands(void)
{
    ti_c54x_multiply_operands operands = base_operands();
    ti_c54x_multiply_result result = {0U, false};

    /* TI SPRU172C MPYU Example 1: uns(0x4000) * uns(0xFE00). */
    operands.unsigned_operands = true;
    operands.select = TI_C54X_MULTIPLY_STORE;
    operands.t_raw16 = UINT16_C(0x4000);
    operands.multiplicand_raw16 = UINT16_C(0xFE00);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x3F800000));
    TI_C54X_TEST_CHECK(!result.overflow);

    /* 0xFFFF * 0xFFFF saturates positive under OVM = 1. */
    operands.t_raw16 = UINT16_C(0xFFFF);
    operands.multiplicand_raw16 = UINT16_C(0xFFFF);
    operands.ovm = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x7FFFFFFF));
    TI_C54X_TEST_CHECK(result.overflow);

    /* OVM = 0 wraps the same unsigned overflow in 40 bits. */
    operands.ovm = UINT8_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0xFFFE0001));
    TI_C54X_TEST_CHECK(result.overflow);

    /* FRCT shifts the unsigned product left by one. */
    operands.t_raw16 = UINT16_C(0x4000);
    operands.multiplicand_raw16 = UINT16_C(0x0002);
    operands.frct = UINT8_C(1);
    TI_C54X_TEST_CHECK(ti_c54x_multiply_accumulate(&operands, &result) ==
                       TI_C54X_MULTIPLY_OK);
    TI_C54X_TEST_CHECK(result.accumulator_raw40 == UINT64_C(0x10000));
    TI_C54X_TEST_CHECK(!result.overflow);
}

int
main(void)
{
    test_ti_mac_example_vectors();
    test_mpy_store_vectors();
    test_rounding_pair();
    test_mas_subtract();
    test_ovm_saturation();
    test_store_ignores_accumulator();
    test_unknown_stops();
    test_invalid_rejects();
    test_unsigned_operands();
    return 0;
}
