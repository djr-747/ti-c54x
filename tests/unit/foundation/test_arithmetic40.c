/*
 * Module contract
 *
 * Owns: executable tests for the neutral forty-bit arithmetic foundation.
 * Does not own: production arithmetic behavior, ISA flags, CPU state, build
 * configuration, or test-harness implementation.
 */

#include "foundation/arithmetic40.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static const ti_c54x_u40_t representative_values[] = {
    UINT64_C(0),
    UINT64_C(1),
    UINT64_C(2),
    UINT64_C(0x7FFFFFFFFF),
    UINT64_C(0x8000000000),
    UINT64_C(0x8000000001),
    UINT64_C(0xFFFFFFFFFE),
    UINT64_C(0xFFFFFFFFFF),
    UINT64_C(0x10000000000),
    UINT64_C(0x10000000001),
    UINT64_C(0xFFFFFFFF00000000),
    UINT64_C(0xAAAAAAAAAAAAAAAA),
    UINT64_C(0xFFFFFFFFFFFFFFFF),
};

static ti_c54x_u40_t
reference_arithmetic_right_shift(ti_c54x_u40_t value, uint64_t count)
{
    const ti_c54x_u40_t canonical = value & TI_C54X_U40_MASK;

    if (count >= TI_C54X_U40_BITS) {
        return (canonical & TI_C54X_U40_SIGN_BIT) != 0U
                   ? TI_C54X_U40_MASK
                   : 0U;
    }

    /* Form the mathematical quotient from magnitude and remainder instead of
     * reproducing the implementation's unsigned shift-and-fill expression.
     * For a negative value, floor(-m / 2^n) is -(m / 2^n rounded up). */
    {
        const uint64_t divisor = UINT64_C(1) << count;
        if ((canonical & TI_C54X_U40_SIGN_BIT) == 0U) {
            return canonical / divisor;
        }

        {
            const uint64_t magnitude =
                TI_C54X_U40_SIGN_BIT -
                (canonical & (TI_C54X_U40_SIGN_BIT - UINT64_C(1)));
            const uint64_t quotient = magnitude / divisor;
            const uint64_t remainder = magnitude % divisor;
            const uint64_t rounded_up = quotient + (remainder != 0U ? 1U : 0U);
            const int64_t negative_quotient = -(int64_t)rounded_up;
            return (uint64_t)negative_quotient & TI_C54X_U40_MASK;
        }
    }
}

static void
test_mask_and_construction(void)
{
    static const uint64_t unsigned_inputs[] = {
        UINT64_C(0), UINT64_C(1), UINT64_C(0xFFFFFFFFFF),
        UINT64_C(0x10000000000), UINT64_C(0x123456789ABCDEF0),
        UINT64_MAX,
    };
    static const int64_t signed_inputs[] = {
        INT64_MIN, -INT64_C(1099511627776), -INT64_C(549755813889),
        -INT64_C(549755813888), -1, 0, 1, INT64_C(549755813887),
        INT64_C(549755813888), INT64_C(1099511627775),
        INT64_C(1099511627776), INT64_MAX,
    };
    size_t i;

    for (i = 0; i < sizeof(unsigned_inputs) / sizeof(unsigned_inputs[0]); ++i) {
        const ti_c54x_u40_t value =
            ti_c54x_u40_from_unsigned(unsigned_inputs[i]);
        TI_C54X_TEST_CHECK_U64(value,
                               unsigned_inputs[i] & TI_C54X_U40_MASK);
        TI_C54X_TEST_CHECK_U64(ti_c54x_u40_mask(value), value);
    }

    for (i = 0; i < sizeof(signed_inputs) / sizeof(signed_inputs[0]); ++i) {
        const ti_c54x_u40_t value =
            ti_c54x_u40_from_signed(signed_inputs[i]);
        const uint64_t expected =
            (uint64_t)signed_inputs[i] & TI_C54X_U40_MASK;
        TI_C54X_TEST_CHECK_U64(value, expected);
        TI_C54X_TEST_CHECK_U64(
            ti_c54x_u40_from_signed(ti_c54x_u40_sign_extend(value)), value);
    }
}

static void
test_sign_extension(void)
{
    static const struct {
        ti_c54x_u40_t bits;
        int64_t value;
    } cases[] = {
        { UINT64_C(0), 0 },
        { UINT64_C(1), 1 },
        { UINT64_C(0x7FFFFFFFFF), INT64_C(549755813887) },
        { UINT64_C(0x8000000000), -INT64_C(549755813888) },
        { UINT64_C(0x8000000001), -INT64_C(549755813887) },
        { UINT64_C(0xFFFFFFFFFE), -2 },
        { UINT64_C(0xFFFFFFFFFF), -1 },
        { UINT64_C(0x10000000000), 0 },
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        TI_C54X_TEST_CHECK(ti_c54x_u40_sign_extend(cases[i].bits) ==
                           cases[i].value);
    }
}

static void
test_shifts(void)
{
    size_t i;

    for (i = 0; i < sizeof(representative_values) /
                        sizeof(representative_values[0]);
         ++i) {
        const ti_c54x_u40_t value = representative_values[i];
        uint64_t count;

        for (count = 0U; count <= TI_C54X_U40_BITS; ++count) {
            ti_c54x_u40_t expected_left = 0U;

            if (count < TI_C54X_U40_BITS) {
                expected_left = (value & TI_C54X_U40_MASK) << count;
                expected_left &= TI_C54X_U40_MASK;
            }

            TI_C54X_TEST_CHECK_U64(
                ti_c54x_u40_arithmetic_right_shift(value, count),
                reference_arithmetic_right_shift(value, count));
            TI_C54X_TEST_CHECK_U64(
                ti_c54x_u40_logical_left_shift(value, count), expected_left);
            TI_C54X_TEST_CHECK(
                ti_c54x_u40_arithmetic_right_shift(value, count) <=
                TI_C54X_U40_MASK);
            TI_C54X_TEST_CHECK(
                ti_c54x_u40_logical_left_shift(value, count) <=
                TI_C54X_U40_MASK);
        }

        /* Also exercise an extreme count that cannot be represented by the
         * architecture's normal immediate fields. */
        TI_C54X_TEST_CHECK_U64(
            ti_c54x_u40_arithmetic_right_shift(value, UINT64_MAX),
            reference_arithmetic_right_shift(value, UINT64_MAX));
        TI_C54X_TEST_CHECK_U64(
            ti_c54x_u40_logical_left_shift(value, UINT64_MAX), 0U);
    }
}

static uint64_t
next_pseudo_random(uint64_t *state)
{
    /* Constants are fixed so failures are reproducible across test runs. */
    *state = *state * UINT64_C(6364136223846793005) +
             UINT64_C(1442695040888963407);
    return *state;
}

static void
test_pseudo_random_invariants(void)
{
    uint64_t state = UINT64_C(0xC54F00D5EED12345);
    size_t i;

    for (i = 0; i < 4096U; ++i) {
        const uint64_t raw = next_pseudo_random(&state);
        const ti_c54x_u40_t canonical = raw & TI_C54X_U40_MASK;
        const uint64_t shift_count = next_pseudo_random(&state) % 65U;
        const ti_c54x_u40_t signed_round_trip = ti_c54x_u40_from_signed(
            ti_c54x_u40_sign_extend(raw));

        TI_C54X_TEST_CHECK_U64(ti_c54x_u40_mask(raw), canonical);
        TI_C54X_TEST_CHECK_U64(ti_c54x_u40_from_unsigned(raw), canonical);
        TI_C54X_TEST_CHECK_U64(ti_c54x_u40_mask(canonical), canonical);
        TI_C54X_TEST_CHECK_U64(signed_round_trip, canonical);
        TI_C54X_TEST_CHECK(
            ti_c54x_u40_arithmetic_right_shift(raw, shift_count) <=
            TI_C54X_U40_MASK);
        TI_C54X_TEST_CHECK(
            ti_c54x_u40_logical_left_shift(raw, shift_count) <=
            TI_C54X_U40_MASK);
        TI_C54X_TEST_CHECK_U64(
            ti_c54x_u40_arithmetic_right_shift(raw, shift_count),
            reference_arithmetic_right_shift(raw, shift_count));
    }
}

int
main(void)
{
    test_mask_and_construction();
    test_sign_extension();
    test_shifts();
    test_pseudo_random_invariants();
    puts("arithmetic40: ok");
    return 0;
}
