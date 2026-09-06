#ifndef TI_C54X_ISA_OP_EFFECT_H
#define TI_C54X_ISA_OP_EFFECT_H

/*
 * Module contract
 *
 * Owns: the private, value-owned description of effects produced by one
 * opcode handler.
 * Does not own: the program counter, CPU/register state, status mutation,
 * memory, buses, timing, sequencing, decode, or observers. NONE means that
 * the opcode owns no architectural or status effect. REPEAT_START carries a
 * copied uint16_t immediate operand only; repeat cardinality, state, PC,
 * timing, and interrupt policy belong to later owners. STATUS_FIELD_WRITE
 * carries a copied field target/value only; mutation belongs to the status
 * sequencer. Callers own each effect object.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_op_effect_kind {
    TI_C54X_OP_EFFECT_NONE = 0,
    TI_C54X_OP_EFFECT_REPEAT_START = 1,
    TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE = 2
} ti_c54x_op_effect_kind;

typedef enum ti_c54x_op_status_field_target {
    TI_C54X_OP_STATUS_FIELD_TARGET_INVALID = 0,
    TI_C54X_OP_STATUS_FIELD_TARGET_DP = 1,
    TI_C54X_OP_STATUS_FIELD_TARGET_ASM = 2,
    TI_C54X_OP_STATUS_FIELD_TARGET_ARP = 3
} ti_c54x_op_status_field_target;

typedef struct ti_c54x_op_repeat_start_effect {
    uint16_t operand;
} ti_c54x_op_repeat_start_effect;

typedef struct ti_c54x_op_status_field_write_effect {
    ti_c54x_op_status_field_target target;
    uint16_t raw_value;
} ti_c54x_op_status_field_write_effect;

typedef union ti_c54x_op_effect_payload {
    ti_c54x_op_repeat_start_effect repeat_start;
    ti_c54x_op_status_field_write_effect status_field_write;
} ti_c54x_op_effect_payload;

typedef struct ti_c54x_op_effect {
    ti_c54x_op_effect_kind kind;
    ti_c54x_op_effect_payload payload;
} ti_c54x_op_effect;

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OP_EFFECT_H */
