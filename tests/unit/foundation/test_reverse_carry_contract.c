/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY tests for reverse-carry arithmetic.
 * The independent oracle covers every uint16_t left operand with each of the
 * sixteen one-hot right operands for both operations; it does not claim the
 * full two-input Cartesian product.
 * Does not own: production arithmetic, TI evidence, address planning, CPU
 * state, build configuration, or test-harness implementation.
 */

#include "foundation/reverse_carry.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

typedef struct oracle_result {
    ti_c54x_reverse_carry_status status;
    uint16_t value;
} oracle_result;

static oracle_result
oracle_reverse_carry(uint16_t lhs, uint16_t rhs, uint8_t subtract)
{
    uint16_t reversed_lhs = UINT16_C(0);
    uint16_t reversed_rhs = UINT16_C(0);
    uint16_t reversed_output;
    uint32_t arithmetic_result;
    unsigned bit;

    /* Bit reversal maps MSB-first carry propagation onto ordinary unsigned
     * arithmetic. This oracle intentionally has a different shape from the
     * production ripple implementation. */
    for (bit = 0U; bit < 16U; ++bit) {
        reversed_lhs |= (uint16_t)((((uint32_t)lhs >> bit) & UINT32_C(1))
                                    << (15U - bit));
        reversed_rhs |= (uint16_t)((((uint32_t)rhs >> bit) & UINT32_C(1))
                                    << (15U - bit));
    }

    if (subtract == UINT8_C(0)) {
        arithmetic_result = (uint32_t)reversed_lhs + (uint32_t)reversed_rhs;
        if (arithmetic_result > UINT32_C(0xffff)) {
            const oracle_result result = {
                .status = TI_C54X_REVERSE_CARRY_STATUS_UNRESOLVED_BOUNDARY,
                .value = UINT16_C(0)
            };
            return result;
        }
    } else if (reversed_lhs < reversed_rhs) {
        const oracle_result result = {
            .status = TI_C54X_REVERSE_CARRY_STATUS_UNRESOLVED_BOUNDARY,
            .value = UINT16_C(0)
        };
        return result;
    } else {
        arithmetic_result = (uint32_t)reversed_lhs - (uint32_t)reversed_rhs;
    }

    reversed_output = (uint16_t)arithmetic_result;
    {
        uint16_t output = UINT16_C(0);
        for (bit = 0U; bit < 16U; ++bit) {
            output |= (uint16_t)((((uint32_t)reversed_output >> bit) &
                                   UINT32_C(1))
                                  << (15U - bit));
        }

        {
            const oracle_result result = {
                .status = TI_C54X_REVERSE_CARRY_STATUS_OK,
                .value = output
            };
            return result;
        }
    }
}

static void
test_explicit_escape_and_invalid_operation(void)
{
    ti_c54x_reverse_carry_result result;

    result = ti_c54x_reverse_carry_add16(UINT16_C(0xffff), UINT16_C(1));
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_REVERSE_CARRY_STATUS_UNRESOLVED_BOUNDARY);
    TI_C54X_TEST_CHECK_U64(result.value, UINT16_C(0));

    result = ti_c54x_reverse_carry_subtract16(UINT16_C(0), UINT16_C(1));
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_REVERSE_CARRY_STATUS_UNRESOLVED_BOUNDARY);
    TI_C54X_TEST_CHECK_U64(result.value, UINT16_C(0));

    result = ti_c54x_reverse_carry_apply16(
        UINT16_C(0x1234), UINT16_C(0x0001),
        (ti_c54x_reverse_carry_operation)UINT8_C(2));
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_REVERSE_CARRY_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK_U64(result.value, UINT16_C(0));
}

static void
check_operation(uint8_t subtract)
{
    uint32_t lhs_value;
    for (lhs_value = UINT32_C(0); lhs_value <= UINT32_C(0xffff);
         ++lhs_value) {
        uint8_t bit;
        for (bit = UINT8_C(0); bit < UINT8_C(16); ++bit) {
            const uint16_t lhs = (uint16_t)lhs_value;
            const uint16_t rhs = (uint16_t)(UINT32_C(1) << bit);
            const oracle_result expected = oracle_reverse_carry(
                lhs, rhs, subtract);
            const ti_c54x_reverse_carry_result actual =
                ti_c54x_reverse_carry_apply16(
                    lhs, rhs,
                    subtract == UINT8_C(0)
                        ? TI_C54X_REVERSE_CARRY_OPERATION_ADD
                        : TI_C54X_REVERSE_CARRY_OPERATION_SUBTRACT);

            TI_C54X_TEST_CHECK(actual.status == expected.status);
            TI_C54X_TEST_CHECK_U64(actual.value, expected.value);
            if (actual.status != TI_C54X_REVERSE_CARRY_STATUS_OK) {
                TI_C54X_TEST_CHECK_U64(actual.value, UINT16_C(0));
            }
        }
    }
}

int
main(void)
{
    test_explicit_escape_and_invalid_operation();
    check_operation(UINT8_C(0));
    check_operation(UINT8_C(1));
    puts("reverse_carry_contract: ok");
    return 0;
}
