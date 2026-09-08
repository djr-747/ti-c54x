/*
 * Module contract
 *
 * Owns: checked and trusted pure reverse-carry MOD4/MOD7 two-word WORD16
 * immediate-store leaves. It copies the extension word, rejects the
 * unresolved ARF0 alias, and stages one generic reverse-carry write.
 * Does not own: catalog generation, decode tables, address resolution, buses,
 * CPU/state mutation, sequencing, timing, observers, diagnostics, allocation,
 * logging, environment, or callbacks.
 */

#include "op_st_immediate_smem.h"

#include <stddef.h>

#include "op_store_reverse_carry_common.h"

static ti_c54x_op_status st_immediate_reverse_values(
    const ti_c54x_store_reverse_carry_operands *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->arf == UINT8_C(0) || operands->arf > UINT8_C(7) ||
        (operands->mode != UINT8_C(4) && operands->mode != UINT8_C(7))) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    ti_c54x_store_stage_reverse_carry_write(
        operands->arf, (ti_c54x_indirect_reverse_carry_mode)operands->mode,
        operands->extension, effect);
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_immediate_reverse_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id,
    ti_c54x_indirect_reverse_carry_mode expected_mode)
{
    ti_c54x_store_reverse_carry_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_store_validate_reverse_carry_instruction(
        instruction, expected_id,
        TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,
        TI_C54X_INPUT_RESOURCES_NONE, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_immediate_reverse_values(&operands, input, effect);
}

static ti_c54x_op_status st_immediate_reverse_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id,
    ti_c54x_indirect_reverse_carry_mode expected_mode)
{
    ti_c54x_store_reverse_carry_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || trusted_operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_store_validate_reverse_carry_trusted_operands(
        instruction, trusted_operands, expected_id,
        TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,
        TI_C54X_INPUT_RESOURCES_NONE, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_immediate_reverse_values(&operands, input, effect);
}

ti_c54x_op_status ti_c54x_op_st_immediate_smem_mod4(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_immediate_reverse_checked(
        instruction, input, effect, TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD4,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD4);
}

ti_c54x_op_status ti_c54x_op_st_immediate_smem_mod7(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_immediate_reverse_checked(
        instruction, input, effect, TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD7,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7);
}

ti_c54x_op_status ti_c54x_trusted_op_st_immediate_smem_mod4(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_immediate_reverse_trusted(
        instruction, trusted_operands, input, effect,
        TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD4,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD4);
}

ti_c54x_op_status ti_c54x_trusted_op_st_immediate_smem_mod7(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_immediate_reverse_trusted(
        instruction, trusted_operands, input, effect,
        TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD7,
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7);
}
