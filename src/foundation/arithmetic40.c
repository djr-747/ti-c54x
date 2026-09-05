/*
 * Module contract
 *
 * Owns: the implementation of the neutral forty-bit numeric helpers declared
 * by foundation/arithmetic40.h.
 * Does not own: ISA flags, accumulator policy, CPU state, memory, or opcode
 * behavior.
 */

#include "arithmetic40.h"

ti_c54x_u40_t
ti_c54x_u40_mask(uint64_t value)
{
    return value & TI_C54X_U40_MASK;
}

ti_c54x_u40_t
ti_c54x_u40_from_unsigned(uint64_t value)
{
    return ti_c54x_u40_mask(value);
}

ti_c54x_u40_t
ti_c54x_u40_from_signed(int64_t value)
{
    /* Conversion to uint64_t is modulo 2^64; masking then selects the lane. */
    return ti_c54x_u40_mask((uint64_t)value);
}

int64_t
ti_c54x_u40_sign_extend(ti_c54x_u40_t value)
{
    const uint64_t canonical = ti_c54x_u40_mask(value);
    const uint64_t magnitude =
        canonical & (TI_C54X_U40_SIGN_BIT - UINT64_C(1));

    if ((canonical & TI_C54X_U40_SIGN_BIT) == 0U) {
        return (int64_t)canonical;
    }

    /* Both operands are at most 2^40-1, so this subtraction cannot overflow. */
    return (int64_t)magnitude - (INT64_C(1) << 39);
}

ti_c54x_u40_t
ti_c54x_u40_arithmetic_right_shift(ti_c54x_u40_t value, uint64_t count)
{
    const uint64_t canonical = ti_c54x_u40_mask(value);

    if (count == 0U) {
        return canonical;
    }

    if (count >= TI_C54X_U40_BITS) {
        return (canonical & TI_C54X_U40_SIGN_BIT) != 0U
                   ? TI_C54X_U40_MASK
                   : 0U;
    }

    /* Unsigned shifts are defined; explicitly add sign fill instead of using
     * an implementation-defined signed right shift. */
    {
        const uint64_t shifted = canonical >> count;
        const uint64_t sign_fill =
            (canonical & TI_C54X_U40_SIGN_BIT) != 0U
                ? (TI_C54X_U40_MASK ^ (TI_C54X_U40_MASK >> count))
                : 0U;
        return ti_c54x_u40_mask(shifted | sign_fill);
    }
}

ti_c54x_u40_t
ti_c54x_u40_logical_left_shift(ti_c54x_u40_t value, uint64_t count)
{
    const uint64_t canonical = ti_c54x_u40_mask(value);

    if (count >= TI_C54X_U40_BITS) {
        return 0U;
    }

    /* The operand and result are unsigned, so this shift has defined C11
     * semantics.  Masking makes the lane width explicit. */
    return ti_c54x_u40_mask(canonical << count);
}
