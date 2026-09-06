#ifndef TI_C54X_ISA_OP_STATUS_H
#define TI_C54X_ISA_OP_STATUS_H

/*
 * Module contract
 *
 * Owns: stable private status values shared by opcode semantic handlers and
 * their dispatch seam.
 * Does not own: decode classification, catalog identity, CPU state, effects,
 * sequencing, timing, buses, or diagnostics.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_op_status {
    TI_C54X_OP_STATUS_OK = 0,
    TI_C54X_OP_STATUS_INVALID_ARGUMENT = 1,
    TI_C54X_OP_STATUS_INVALID_DECODE = 2,
    TI_C54X_OP_STATUS_UNSUPPORTED_OPCODE = 3,
    TI_C54X_OP_STATUS_INVALID_OPERAND = 4,
    TI_C54X_OP_STATUS_UNKNOWN_INPUT = 5
} ti_c54x_op_status;

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OP_STATUS_H */
