/*
 * Module contract
 *
 * Owns: executable COMMON_TI tests for the reverse-carry primitive, including
 * the documented TI MOD7 sequence and safe inverse MOD4 transitions.
 * Does not own: production arithmetic, address planning, CPU state, build
 * configuration, or test-harness implementation.
 */

#include "foundation/reverse_carry.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static void
test_ti_mod7_sequence(void)
{
    static const uint16_t expected[] = {
        UINT16_C(0x60), UINT16_C(0x68), UINT16_C(0x64), UINT16_C(0x6c),
        UINT16_C(0x62), UINT16_C(0x6a), UINT16_C(0x66), UINT16_C(0x6e)
    };
    size_t step;

    for (step = 0U; step + 1U < sizeof(expected) / sizeof(expected[0]);
         ++step) {
        const ti_c54x_reverse_carry_result result =
            ti_c54x_reverse_carry_add16(expected[step], UINT16_C(0x0008));
        TI_C54X_TEST_CHECK(result.status == TI_C54X_REVERSE_CARRY_STATUS_OK);
        TI_C54X_TEST_CHECK_U64(result.value, expected[step + 1U]);
    }
}

static void
test_safe_mod4_inverses(void)
{
    static const uint16_t expected[] = {
        UINT16_C(0x60), UINT16_C(0x68), UINT16_C(0x64), UINT16_C(0x6c),
        UINT16_C(0x62), UINT16_C(0x6a), UINT16_C(0x66), UINT16_C(0x6e)
    };
    size_t step;

    for (step = 1U; step < sizeof(expected) / sizeof(expected[0]); ++step) {
        const ti_c54x_reverse_carry_result result =
            ti_c54x_reverse_carry_subtract16(expected[step], UINT16_C(0x0008));
        TI_C54X_TEST_CHECK(result.status == TI_C54X_REVERSE_CARRY_STATUS_OK);
        TI_C54X_TEST_CHECK_U64(result.value, expected[step - 1U]);
    }
}

int
main(void)
{
    test_ti_mod7_sequence();
    test_safe_mod4_inverses();
    puts("reverse_carry: ok");
    return 0;
}
