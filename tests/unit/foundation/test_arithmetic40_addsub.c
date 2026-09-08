/*
 * Module contract
 *
 * Owns: executable coverage for checked modulo-2^40 add/subtract and its
 * carry, borrow, and signed-32 overflow reports.
 * Does not own: OVM policy, status mutation, opcode semantics, or timing.
 */

#include "foundation/arithmetic40_addsub.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static void test_add_boundaries(void)
{
    ti_c54x_u40_add_result result;

    TI_C54X_TEST_CHECK(ti_c54x_u40_add_checked(UINT64_C(0), UINT64_C(0),
                                               &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(0));
    TI_C54X_TEST_CHECK(!result.low32_carry);
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_NONE);

    TI_C54X_TEST_CHECK(ti_c54x_u40_add_checked(UINT64_C(0xffffffff), 1U,
                                               &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(0x100000000));
    TI_C54X_TEST_CHECK(result.low32_carry);
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_POSITIVE);

    TI_C54X_TEST_CHECK(ti_c54x_u40_add_checked(UINT64_C(0xffffffffff), 1U,
                                               &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(0));
    TI_C54X_TEST_CHECK(result.low32_carry);
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_NONE);

    TI_C54X_TEST_CHECK(ti_c54x_u40_add_checked(UINT64_C(0x7fffffff), 1U,
                                               &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(0x80000000));
    TI_C54X_TEST_CHECK(!result.low32_carry);
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_POSITIVE);

    TI_C54X_TEST_CHECK(ti_c54x_u40_add_checked(UINT64_C(0x8000000000),
                                               UINT64_C(0x8000000000),
                                               &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(0));
    TI_C54X_TEST_CHECK(!result.low32_carry);
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_NEGATIVE);

    TI_C54X_TEST_CHECK(ti_c54x_u40_add_checked(UINT64_C(0x7fffffffff),
                                               UINT64_C(0x7fffffffff),
                                               &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(0xfffffffffe));
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_POSITIVE);
}

static void test_sub_boundaries(void)
{
    ti_c54x_u40_sub_result result;

    TI_C54X_TEST_CHECK(ti_c54x_u40_sub_checked(UINT64_C(0), UINT64_C(0),
                                               &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(0));
    TI_C54X_TEST_CHECK(result.low32_no_borrow);
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_NONE);

    TI_C54X_TEST_CHECK(ti_c54x_u40_sub_checked(UINT64_C(0), 1U, &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(0xffffffffff));
    TI_C54X_TEST_CHECK(!result.low32_no_borrow);
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_NONE);

    TI_C54X_TEST_CHECK(ti_c54x_u40_sub_checked(UINT64_C(0x80000000), 1U,
                                               &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(0x7fffffff));
    TI_C54X_TEST_CHECK(result.low32_no_borrow);
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_NONE);

    TI_C54X_TEST_CHECK(ti_c54x_u40_sub_checked(UINT64_C(0x8000000000),
                                               UINT64_C(0x7fffffffff),
                                               &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(1));
    TI_C54X_TEST_CHECK(!result.low32_no_borrow);
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_NEGATIVE);

    /* Full-40 borrow can coexist with low-32 no-borrow because both low
     * words are zero; the signed mathematical result is +2^39. */
    TI_C54X_TEST_CHECK(ti_c54x_u40_sub_checked(UINT64_C(0),
                                               UINT64_C(0x8000000000),
                                               &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(0x8000000000));
    TI_C54X_TEST_CHECK(result.low32_no_borrow);
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_POSITIVE);

    TI_C54X_TEST_CHECK(ti_c54x_u40_sub_checked(UINT64_C(0x7fffffffff),
                                               UINT64_C(0x8000000000),
                                               &result) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_OK);
    TI_C54X_TEST_CHECK_U64(result.value, UINT64_C(0xffffffffff));
    TI_C54X_TEST_CHECK(result.signed32_overflow ==
                       TI_C54X_SIGNED32_OVERFLOW_POSITIVE);
}

static void test_invalid_is_transactional(void)
{
    ti_c54x_u40_add_result add = {
        UINT64_C(0x12345), true, TI_C54X_SIGNED32_OVERFLOW_NEGATIVE};
    ti_c54x_u40_sub_result sub = {
        UINT64_C(0x23456), false, TI_C54X_SIGNED32_OVERFLOW_POSITIVE};
    const ti_c54x_u40_add_result add_before = add;
    const ti_c54x_u40_sub_result sub_before = sub;

    TI_C54X_TEST_CHECK(ti_c54x_u40_add_checked(UINT64_MAX, 0,
                                               &add) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&add, &add_before, sizeof(add)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_u40_sub_checked(0, UINT64_MAX,
                                               &sub) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&sub, &sub_before, sizeof(sub)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_u40_add_checked(0, 0, NULL) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_u40_sub_checked(0, 0, NULL) ==
                       TI_C54X_ARITHMETIC40_ADDSUB_INVALID_ARGUMENT);
}

int main(void)
{
    test_add_boundaries();
    test_sub_boundaries();
    test_invalid_is_transactional();
    return 0;
}
