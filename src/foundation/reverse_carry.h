#ifndef TI_C54X_FOUNDATION_REVERSE_CARRY_H
#define TI_C54X_FOUNDATION_REVERSE_CARRY_H

/*
 * Module contract
 *
 * Owns: a pure unsigned 16-bit reverse-carry add/subtract primitive. Carry
 * or borrow propagates from bit 15 toward bit 0, and final propagation out of
 * bit 0 is reported instead of being silently wrapped.
 * Does not own: CPU state, address registers, circular or modulo addressing,
 * status flags, memory, allocation, diagnostics, or environment state.
 * Inputs are copied scalar values. Results are returned by value and retain
 * no caller-owned storage. Every non-OK result has a zero value.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_reverse_carry_operation {
    TI_C54X_REVERSE_CARRY_OPERATION_ADD = 0,
    TI_C54X_REVERSE_CARRY_OPERATION_SUBTRACT = 1
} ti_c54x_reverse_carry_operation;

typedef enum ti_c54x_reverse_carry_status {
    TI_C54X_REVERSE_CARRY_STATUS_OK = 0,
    TI_C54X_REVERSE_CARRY_STATUS_INVALID_ARGUMENT = 1,
    TI_C54X_REVERSE_CARRY_STATUS_UNRESOLVED_BOUNDARY = 2
} ti_c54x_reverse_carry_status;

typedef struct ti_c54x_reverse_carry_result {
    ti_c54x_reverse_carry_status status;
    uint16_t value;
} ti_c54x_reverse_carry_result;

/* Apply one reverse-carry operation to two copied unsigned 16-bit operands. */
ti_c54x_reverse_carry_result ti_c54x_reverse_carry_apply16(
    uint16_t lhs,
    uint16_t rhs,
    ti_c54x_reverse_carry_operation operation);

/* Convenience entry points for the two supported operations. */
ti_c54x_reverse_carry_result ti_c54x_reverse_carry_add16(uint16_t lhs,
                                                          uint16_t rhs);
ti_c54x_reverse_carry_result ti_c54x_reverse_carry_subtract16(uint16_t lhs,
                                                               uint16_t rhs);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_FOUNDATION_REVERSE_CARRY_H */
