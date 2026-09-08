/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for standard-indirect WORD16
 * indirect loads into accumulator A or B, including the unshifted,
 * SXM-dependent logical-left-shift-16, and T-register-shift forms. Each
 * checked leaf validates its catalog/decode contract and stages a generic
 * memory-read effect; its static
 * completion validates the copied instruction and input before staging the
 * accumulator write. The trusted leaves consume constructor-validated
 * operands while retaining SXM input validation.
 * Does not own: opcode inventory, generic decode, AR resolution or mutation,
 * buses, CPU/state mutation, sequencing, timing, observers, allocation,
 * logging, environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained; failed calls
 * leave the caller-owned effect unchanged.
 */

#include "op_ld_smem.h"

#include <stdbool.h>
#include <stddef.h>

#include "foundation/arithmetic40.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_ld_smem_common.h"

static ti_c54x_op_status ld_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

static ti_c54x_op_status ld_smem_shift16_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

static ti_c54x_op_status ld_smem_ts_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

static ti_c54x_op_status ld_smem_validate_sxm(
    const ti_c54x_op_input_snapshot *input)
{
    if (!input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_sxm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_smem_complete_values(
    uint8_t destination,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    uint8_t shift,
    bool arithmetic_right,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_op_status input_status = ld_smem_validate_sxm(input);
    ti_c54x_op_effect staged;
    ti_c54x_u40_t value_raw40;

    if (input_status != TI_C54X_OP_STATUS_OK) {
        return input_status;
    }

    value_raw40 = ti_c54x_u40_from_unsigned(read_value_word);
    if (input->status_sxm != UINT8_C(0) &&
        (read_value_word & UINT16_C(0x8000)) != UINT16_C(0)) {
        value_raw40 = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                      (uint64_t)read_value_word;
    }
    value_raw40 = arithmetic_right
                      ? ti_c54x_u40_arithmetic_right_shift(value_raw40, shift)
                      : ti_c54x_u40_logical_left_shift(value_raw40, shift);
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = value_raw40,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE}}};
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_smem_ts_validate_input(
    const ti_c54x_op_input_snapshot *input)
{
    const ti_c54x_op_status sxm_status = ld_smem_validate_sxm(input);
    const uint8_t shift_bits =
        (uint8_t)(input->t_register_raw16 & UINT16_C(0x3F));

    if (sxm_status != TI_C54X_OP_STATUS_OK) {
        return sxm_status;
    }
    if (!input->t_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (shift_bits >= UINT8_C(0x20) && shift_bits <= UINT8_C(0x2F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_smem_ts_complete_values(
    uint8_t destination,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    const ti_c54x_op_status input_status = ld_smem_ts_validate_input(input);
    const uint8_t shift_bits =
        (uint8_t)(input->t_register_raw16 & UINT16_C(0x3F));

    if (input_status != TI_C54X_OP_STATUS_OK) {
        return input_status;
    }
    return ld_smem_complete_values(
        destination, input, read_value_word, shift_bits <= UINT8_C(31)
                                                 ? shift_bits
                                                 : (uint8_t)(UINT8_C(64) -
                                                             shift_bits),
        shift_bits > UINT8_C(31), next_effect);
}

static ti_c54x_op_status ld_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_instruction(
        instruction, TI_C54X_OPCODE_ID_LD_SMEM,
        TI_C54X_INPUT_RESOURCES_STATUS_SXM, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ld_smem_complete_values(operands.destination, input, read_value_word, 0U,
                                   false, next_effect);
}

static ti_c54x_op_status ld_smem_shift16_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_instruction(
        instruction, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16,
        TI_C54X_INPUT_RESOURCES_STATUS_SXM, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ld_smem_complete_values(operands.destination, input, read_value_word, 16U,
                                   false, next_effect);
}

static ti_c54x_op_status ld_smem_ts_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_instruction(
        instruction, TI_C54X_OPCODE_ID_LD_SMEM_TS,
        TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ld_smem_ts_complete_values(operands.destination, input, read_value_word,
                                      next_effect);
}

ti_c54x_op_status ti_c54x_op_ld_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_instruction(
        instruction, TI_C54X_OPCODE_ID_LD_SMEM,
        TI_C54X_INPUT_RESOURCES_STATUS_SXM, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_smem_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_memory_read(&operands, ld_smem_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ld_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands decoded_operands;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_trusted_operands(operands,
                                                       &decoded_operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(decoded_operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_smem_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_memory_read(&decoded_operands, ld_smem_complete,
                                      effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_ld_smem_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_instruction(
        instruction, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16,
        TI_C54X_INPUT_RESOURCES_STATUS_SXM, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_smem_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_memory_read(&operands, ld_smem_shift16_complete,
                                      effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_ld_smem_ts(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_instruction(
        instruction, TI_C54X_OPCODE_ID_LD_SMEM_TS,
        TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_smem_ts_validate_input(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_memory_read(&operands, ld_smem_ts_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ld_smem_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands decoded_operands;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_trusted_operands(operands,
                                                       &decoded_operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(decoded_operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_smem_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_memory_read(&decoded_operands,
                                      ld_smem_shift16_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ld_smem_ts(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands decoded_operands;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_trusted_operands(operands,
                                                       &decoded_operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(decoded_operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_smem_ts_validate_input(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_memory_read(&decoded_operands, ld_smem_ts_complete,
                                      effect);
    return TI_C54X_OP_STATUS_OK;
}
