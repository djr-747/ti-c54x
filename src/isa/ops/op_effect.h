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
 * sequencer. ACCUMULATOR_WRITE carries only a canonical destination value
 * and explicit C/TC status disposition; mutation belongs to its sequencer.
 * T_REGISTER_WRITE carries one copied 16-bit T value; mutation belongs to
 * the temporary-register sequencer.
 * Callers own each effect object.
 */

#include <stdint.h>

#include "foundation/arithmetic40.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_op_effect_kind {
    TI_C54X_OP_EFFECT_NONE = 0,
    TI_C54X_OP_EFFECT_REPEAT_START = 1,
    TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE = 2,
    TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE = 3,
    TI_C54X_OP_EFFECT_T_REGISTER_WRITE = 4
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

typedef enum ti_c54x_op_status_bit_action {
    TI_C54X_OP_STATUS_BIT_PRESERVE = 0,
    TI_C54X_OP_STATUS_BIT_WRITE_0 = 1,
    TI_C54X_OP_STATUS_BIT_WRITE_1 = 2,
    TI_C54X_OP_STATUS_BIT_ACTION_COUNT = 3
} ti_c54x_op_status_bit_action;

typedef struct ti_c54x_op_status_delta {
    ti_c54x_op_status_bit_action c_action;
    ti_c54x_op_status_bit_action tc_action;
} ti_c54x_op_status_delta;

typedef struct ti_c54x_op_accumulator_write_effect {
    uint8_t destination;
    ti_c54x_u40_t value_raw40;
    ti_c54x_op_status_delta status_delta;
} ti_c54x_op_accumulator_write_effect;

typedef struct ti_c54x_op_t_register_write_effect {
    uint16_t raw_value;
} ti_c54x_op_t_register_write_effect;

typedef union ti_c54x_op_effect_payload {
    ti_c54x_op_repeat_start_effect repeat_start;
    ti_c54x_op_status_field_write_effect status_field_write;
    ti_c54x_op_accumulator_write_effect accumulator_write;
    ti_c54x_op_t_register_write_effect t_register_write;
} ti_c54x_op_effect_payload;

typedef struct ti_c54x_op_effect {
    ti_c54x_op_effect_kind kind;
    ti_c54x_op_effect_payload payload;
} ti_c54x_op_effect;

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OP_EFFECT_H */
