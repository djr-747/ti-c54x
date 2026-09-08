#ifndef TI_C54X_FOUNDATION_ARITHMETIC40_ADDSUB_H
#define TI_C54X_FOUNDATION_ARITHMETIC40_ADDSUB_H

/* Module contract
 * Owns: checked modulo-2^40 unsigned add/subtract and independent carry,
 * borrow, and signed-32 overflow-direction reports.
 * Does not own: OVM/OVA policy, status mutation, opcode semantics, or timing.
 */

#include "arithmetic40.h"

#include <stdbool.h>

typedef enum ti_c54x_arithmetic40_addsub_status {
    TI_C54X_ARITHMETIC40_ADDSUB_OK = 0,
    TI_C54X_ARITHMETIC40_ADDSUB_INVALID_ARGUMENT = 1
} ti_c54x_arithmetic40_addsub_status;

typedef enum ti_c54x_signed32_overflow_direction {
    TI_C54X_SIGNED32_OVERFLOW_NONE = 0,
    TI_C54X_SIGNED32_OVERFLOW_POSITIVE = 1,
    TI_C54X_SIGNED32_OVERFLOW_NEGATIVE = 2
} ti_c54x_signed32_overflow_direction;

typedef struct ti_c54x_u40_add_result {
    ti_c54x_u40_t value;
    bool low32_carry;
    ti_c54x_signed32_overflow_direction signed32_overflow;
} ti_c54x_u40_add_result;

typedef struct ti_c54x_u40_sub_result {
    ti_c54x_u40_t value;
    bool low32_no_borrow;
    ti_c54x_signed32_overflow_direction signed32_overflow;
} ti_c54x_u40_sub_result;

ti_c54x_arithmetic40_addsub_status ti_c54x_u40_add_checked(
    ti_c54x_u40_t lhs, ti_c54x_u40_t rhs, ti_c54x_u40_add_result *output);

ti_c54x_arithmetic40_addsub_status ti_c54x_u40_sub_checked(
    ti_c54x_u40_t lhs, ti_c54x_u40_t rhs, ti_c54x_u40_sub_result *output);

#endif /* TI_C54X_FOUNDATION_ARITHMETIC40_ADDSUB_H */
