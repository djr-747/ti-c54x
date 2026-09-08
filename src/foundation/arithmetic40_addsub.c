/* Module contract
 * Owns: checked modulo-2^40 unsigned add/subtract and independent carry,
 * borrow, and signed-32 overflow-direction reports.
 * Does not own: OVM/OVA policy, status mutation, opcode semantics, or timing.
 */

#include "arithmetic40_addsub.h"

#include <stddef.h>
#include <stdint.h>

static int64_t signed40(ti_c54x_u40_t value)
{
    if ((value & TI_C54X_U40_SIGN_BIT) != 0U) {
        return -(int64_t)((UINT64_C(1) << TI_C54X_U40_BITS) - value);
    }
    return (int64_t)value;
}

static ti_c54x_signed32_overflow_direction signed32_direction(int64_t value)
{
    if (value > INT64_C(2147483647)) {
        return TI_C54X_SIGNED32_OVERFLOW_POSITIVE;
    }
    if (value < INT64_C(-2147483648)) {
        return TI_C54X_SIGNED32_OVERFLOW_NEGATIVE;
    }
    return TI_C54X_SIGNED32_OVERFLOW_NONE;
}

ti_c54x_arithmetic40_addsub_status ti_c54x_u40_add_checked(
    ti_c54x_u40_t lhs, ti_c54x_u40_t rhs, ti_c54x_u40_add_result *output)
{
    ti_c54x_u40_add_result staged;
    uint64_t low_sum;
    int64_t mathematical;

    if (output == NULL || lhs > TI_C54X_U40_MASK ||
        rhs > TI_C54X_U40_MASK) {
        return TI_C54X_ARITHMETIC40_ADDSUB_INVALID_ARGUMENT;
    }
    low_sum = (lhs & UINT64_C(0xffffffff)) +
              (rhs & UINT64_C(0xffffffff));
    mathematical = signed40(lhs) + signed40(rhs);
    staged.value = (lhs + rhs) & TI_C54X_U40_MASK;
    staged.low32_carry = low_sum > UINT64_C(0xffffffff);
    staged.signed32_overflow = signed32_direction(mathematical);
    *output = staged;
    return TI_C54X_ARITHMETIC40_ADDSUB_OK;
}

ti_c54x_arithmetic40_addsub_status ti_c54x_u40_sub_checked(
    ti_c54x_u40_t lhs, ti_c54x_u40_t rhs, ti_c54x_u40_sub_result *output)
{
    ti_c54x_u40_sub_result staged;
    int64_t mathematical;

    if (output == NULL || lhs > TI_C54X_U40_MASK ||
        rhs > TI_C54X_U40_MASK) {
        return TI_C54X_ARITHMETIC40_ADDSUB_INVALID_ARGUMENT;
    }
    mathematical = signed40(lhs) - signed40(rhs);
    staged.value = (lhs - rhs) & TI_C54X_U40_MASK;
    staged.low32_no_borrow =
        (lhs & UINT64_C(0xffffffff)) >= (rhs & UINT64_C(0xffffffff));
    staged.signed32_overflow = signed32_direction(mathematical);
    *output = staged;
    return TI_C54X_ARITHMETIC40_ADDSUB_OK;
}
