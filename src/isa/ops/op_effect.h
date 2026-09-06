#ifndef TI_C54X_ISA_OP_EFFECT_H
#define TI_C54X_ISA_OP_EFFECT_H

/*
 * Module contract
 *
 * Owns: the private, value-owned description of effects produced by one
 * opcode handler.
 * Does not own: the program counter, CPU/register state, status mutation,
 * memory, buses, timing, sequencing, decode, or observers. NONE is the
 * initial and only currently valid effect; it means that the opcode owns no
 * architectural or status effect. Callers own each effect object.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_op_effect_kind {
    TI_C54X_OP_EFFECT_NONE = 0
} ti_c54x_op_effect_kind;

typedef struct ti_c54x_op_effect {
    ti_c54x_op_effect_kind kind;
} ti_c54x_op_effect;

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OP_EFFECT_H */
