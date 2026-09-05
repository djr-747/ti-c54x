#ifndef TI_C54X_FOUNDATION_ARITHMETIC40_H
#define TI_C54X_FOUNDATION_ARITHMETIC40_H

/*
 * Module contract
 *
 * Owns: canonical storage, construction, sign interpretation, and
 * width-defined shifts for unsigned representations of forty-bit values.
 * Does not own: ISA flags, accumulator policy, saturation/rounding policy,
 * CPU state, memory, or instruction semantics.
 *
 * A ti_c54x_u40_t stores a C54x lane in a uint64_t.  Only its low forty bits
 * are defined; functions in this module return canonical values with all
 * other bits clear.  The unsigned representation is deliberate: C's signed
 * overflow and signed right-shift rules are not part of the DSP architecture.
 */

#include <stdint.h>

#define TI_C54X_U40_BITS UINT64_C(40)
#define TI_C54X_U40_MASK UINT64_C(0xFFFFFFFFFF)
#define TI_C54X_U40_SIGN_BIT UINT64_C(0x8000000000)

typedef uint64_t ti_c54x_u40_t;

/* Return the canonical low-forty-bit representation of value. */
ti_c54x_u40_t ti_c54x_u40_mask(uint64_t value);

/* Construct a low-forty-bit value, retaining two's-complement modulo bits. */
ti_c54x_u40_t ti_c54x_u40_from_unsigned(uint64_t value);
ti_c54x_u40_t ti_c54x_u40_from_signed(int64_t value);

/* Interpret a canonical (or non-canonical) value as a signed 40-bit integer. */
int64_t ti_c54x_u40_sign_extend(ti_c54x_u40_t value);

/*
 * Shift operations on the forty-bit lane.
 *
 * Counts are uint64_t so every possible caller-provided count is defined:
 * arithmetic right shift by >= 40 produces the sign fill, and logical left
 * shift by >= 40 produces zero.  Results are always canonical low-forty-bit
 * values.
 */
ti_c54x_u40_t ti_c54x_u40_arithmetic_right_shift(ti_c54x_u40_t value,
                                                  uint64_t count);
ti_c54x_u40_t ti_c54x_u40_logical_left_shift(ti_c54x_u40_t value,
                                              uint64_t count);

#endif /* TI_C54X_FOUNDATION_ARITHMETIC40_H */
