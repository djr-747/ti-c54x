/*
 * Module contract
 *
 * Owns: the implementation of the pure unsigned 16-bit reverse-carry
 * primitive declared by foundation/reverse_carry.h.
 * Does not own: CPU state, address generation, status flags, memory,
 * allocation, diagnostics, or environment state.
 */

#include "reverse_carry.h"

#include <stdbool.h>
#include <stdint.h>

static ti_c54x_reverse_carry_result
ti_c54x_reverse_carry_result_with_status(ti_c54x_reverse_carry_status status)
{
    const ti_c54x_reverse_carry_result result = {
        .status = status,
        .value = UINT16_C(0)
    };
    return result;
}

static ti_c54x_reverse_carry_result
ti_c54x_reverse_carry_calculate(uint16_t lhs,
                                uint16_t rhs,
                                bool subtract)
{
    uint16_t value = UINT16_C(0);
    uint8_t carry_or_borrow = UINT8_C(0);
    unsigned bit;

    /* The loop intentionally walks from the most-significant bit down. */
    for (bit = 16U; bit != 0U; --bit) {
        const unsigned shift = bit - 1U;
        const uint8_t lhs_bit = (uint8_t)(((uint32_t)lhs >> shift) &
                                          UINT32_C(1));
        const uint8_t rhs_bit = (uint8_t)(((uint32_t)rhs >> shift) &
                                          UINT32_C(1));
        uint8_t output_bit;

        if (!subtract) {
            const uint8_t total = (uint8_t)(lhs_bit + rhs_bit +
                                             carry_or_borrow);
            output_bit = (uint8_t)(total & UINT8_C(1));
            carry_or_borrow = (uint8_t)(total >> 1U);
        } else {
            const uint8_t required = (uint8_t)(rhs_bit + carry_or_borrow);
            if (lhs_bit >= required) {
                output_bit = (uint8_t)(lhs_bit - required);
                carry_or_borrow = UINT8_C(0);
            } else {
                output_bit = (uint8_t)(UINT8_C(2) + lhs_bit - required);
                carry_or_borrow = UINT8_C(1);
            }
        }

        if (output_bit != UINT8_C(0)) {
            value = (uint16_t)(value |
                               (uint16_t)(UINT32_C(1) << shift));
        }
    }

    if (carry_or_borrow != UINT8_C(0)) {
        return ti_c54x_reverse_carry_result_with_status(
            TI_C54X_REVERSE_CARRY_STATUS_UNRESOLVED_BOUNDARY);
    }

    {
        const ti_c54x_reverse_carry_result result = {
            .status = TI_C54X_REVERSE_CARRY_STATUS_OK,
            .value = value
        };
        return result;
    }
}

ti_c54x_reverse_carry_result
ti_c54x_reverse_carry_apply16(uint16_t lhs,
                              uint16_t rhs,
                              ti_c54x_reverse_carry_operation operation)
{
    if (operation == TI_C54X_REVERSE_CARRY_OPERATION_ADD) {
        return ti_c54x_reverse_carry_calculate(lhs, rhs, false);
    }
    if (operation == TI_C54X_REVERSE_CARRY_OPERATION_SUBTRACT) {
        return ti_c54x_reverse_carry_calculate(lhs, rhs, true);
    }
    return ti_c54x_reverse_carry_result_with_status(
        TI_C54X_REVERSE_CARRY_STATUS_INVALID_ARGUMENT);
}

ti_c54x_reverse_carry_result
ti_c54x_reverse_carry_add16(uint16_t lhs, uint16_t rhs)
{
    return ti_c54x_reverse_carry_apply16(
        lhs, rhs, TI_C54X_REVERSE_CARRY_OPERATION_ADD);
}

ti_c54x_reverse_carry_result
ti_c54x_reverse_carry_subtract16(uint16_t lhs, uint16_t rhs)
{
    return ti_c54x_reverse_carry_apply16(
        lhs, rhs, TI_C54X_REVERSE_CARRY_OPERATION_SUBTRACT);
}
