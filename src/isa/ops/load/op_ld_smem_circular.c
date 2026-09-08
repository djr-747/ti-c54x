/*
 * Module contract
 *
 * Owns: checked and trusted circular MOD8/MOD10 WORD16 leaves for LD, LD TS,
 * and LD shift16. Validation/staging is delegated to the private common seam;
 * this file owns only family completion transforms and wrappers.
 * Does not own: catalog/decode definitions, address resolution or mutation, buses, CPU
 * state, sequencing, timing, observers, allocation, logging, environment, or
 * diagnostics. Borrowed objects are used for one call and no pointer is kept.
 */

#include "op_ld_smem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_ld_smem_common.h"

typedef enum circular_transform {
    CIRCULAR_SIGNED = 0,
    CIRCULAR_SHIFT16,
    CIRCULAR_TS
} circular_transform;

static ti_c54x_op_status validate_sxm(
    const ti_c54x_op_input_snapshot *input)
{
    if (!input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    return input->status_sxm <= UINT8_C(1)
               ? TI_C54X_OP_STATUS_OK
               : TI_C54X_OP_STATUS_INVALID_OPERAND;
}

static ti_c54x_op_status validate_ts(
    const ti_c54x_op_input_snapshot *input)
{
    uint8_t shift;
    ti_c54x_op_status status;

    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (!input->t_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    shift = (uint8_t)(input->t_register_raw16 & UINT16_C(0x3F));
    return shift >= UINT8_C(0x20) && shift <= UINT8_C(0x2F)
               ? TI_C54X_OP_STATUS_INVALID_OPERAND
               : TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status complete_values(
    uint8_t destination, const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word, uint8_t shift, bool arithmetic_right,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_u40_t value_raw40;
    ti_c54x_op_status status;

    if (input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
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
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = value_raw40,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE}}};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status complete_ts(
    uint8_t destination, const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word, ti_c54x_op_effect *next_effect)
{
    uint8_t shift;
    ti_c54x_op_status status = validate_ts(input);

    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    shift = (uint8_t)(input->t_register_raw16 & UINT16_C(0x3F));
    return complete_values(destination, input, read_value_word,
                           shift <= UINT8_C(31)
                               ? shift
                               : (uint8_t)(UINT8_C(64) - shift),
                           shift > UINT8_C(31), next_effect);
}

static ti_c54x_op_status circular_complete(
    ti_c54x_opcode_id expected_id, ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_input_resource_mask resources, circular_transform transform,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_circular_instruction(
        instruction, expected_id, resources,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    switch (transform) {
    case CIRCULAR_SIGNED:
        return complete_values(operands.destination, input, read_value_word,
                               UINT8_C(0), false, next_effect);
    case CIRCULAR_SHIFT16:
        return complete_values(operands.destination, input, read_value_word,
                               UINT8_C(16), false, next_effect);
    case CIRCULAR_TS:
        return complete_ts(operands.destination, input, read_value_word,
                           next_effect);
    }
    return TI_C54X_OP_STATUS_INVALID_OPERAND;
}

static ti_c54x_op_status checked_circular(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_opcode_id expected_id,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_input_resource_mask resources, circular_transform transform,
    ti_c54x_op_memory_complete_fn complete, ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_circular_instruction(
        instruction, expected_id, resources,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = transform == CIRCULAR_TS ? validate_ts(input) : validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_circular_memory_read(
        &operands, expected_mode, complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status trusted_circular(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_opcode_id expected_id,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_input_resource_mask resources, circular_transform transform,
    ti_c54x_op_memory_complete_fn complete, ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || trusted_operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_circular_trusted_operands(
        instruction, trusted_operands, expected_id, resources,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = transform == CIRCULAR_TS ? validate_ts(input) : validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_circular_memory_read(
        &operands, expected_mode, complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

#define DEFINE_CALLBACK(suffix, id, mode, resources, transform)                 \
    static ti_c54x_op_status circular_##suffix##_complete(                      \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,        \
        ti_c54x_op_effect *next_effect)                                          \
    {                                                                             \
        return circular_complete(id, mode, resources, transform, instruction,    \
                                 input, read_value_word, next_effect);            \
    }

#define DEFINE_WRAPPER(suffix, id, mode, resources, transform)                  \
    ti_c54x_op_status ti_c54x_op_ld_smem_##suffix(                               \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)       \
    {                                                                             \
        return checked_circular(instruction, input, id, mode, resources,         \
                                transform, circular_##suffix##_complete, effect); \
    }                                                                             \
    ti_c54x_op_status ti_c54x_trusted_op_ld_smem_##suffix(                       \
        const ti_c54x_decoded_instruction *instruction,                          \
        const ti_c54x_trusted_operand_values *operands,                          \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)        \
    {                                                                             \
        return trusted_circular(instruction, operands, input, id, mode,          \
                                resources, transform,                             \
                                circular_##suffix##_complete, effect);             \
    }

DEFINE_CALLBACK(mod8, TI_C54X_OPCODE_ID_LD_SMEM_MOD8,
                TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
                CIRCULAR_SIGNED)
DEFINE_CALLBACK(mod10, TI_C54X_OPCODE_ID_LD_SMEM_MOD10,
                TI_C54X_INDIRECT_CIRCULAR_MOD10, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
                CIRCULAR_SIGNED)
DEFINE_CALLBACK(ts_mod8, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD8,
                TI_C54X_INDIRECT_CIRCULAR_MOD8,
                TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, CIRCULAR_TS)
DEFINE_CALLBACK(ts_mod10, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD10,
                TI_C54X_INDIRECT_CIRCULAR_MOD10,
                TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, CIRCULAR_TS)
DEFINE_CALLBACK(shift16_mod8, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD8,
                TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
                CIRCULAR_SHIFT16)
DEFINE_CALLBACK(shift16_mod10, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD10,
                TI_C54X_INDIRECT_CIRCULAR_MOD10, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
                CIRCULAR_SHIFT16)

DEFINE_WRAPPER(mod8, TI_C54X_OPCODE_ID_LD_SMEM_MOD8,
               TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
               CIRCULAR_SIGNED)
DEFINE_WRAPPER(mod10, TI_C54X_OPCODE_ID_LD_SMEM_MOD10,
               TI_C54X_INDIRECT_CIRCULAR_MOD10, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
               CIRCULAR_SIGNED)
DEFINE_WRAPPER(ts_mod8, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD8,
               TI_C54X_INDIRECT_CIRCULAR_MOD8,
               TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, CIRCULAR_TS)
DEFINE_WRAPPER(ts_mod10, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD10,
               TI_C54X_INDIRECT_CIRCULAR_MOD10,
               TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, CIRCULAR_TS)
DEFINE_WRAPPER(shift16_mod8, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD8,
               TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
               CIRCULAR_SHIFT16)
DEFINE_WRAPPER(shift16_mod10, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD10,
               TI_C54X_INDIRECT_CIRCULAR_MOD10, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
               CIRCULAR_SHIFT16)

#undef DEFINE_WRAPPER
#undef DEFINE_CALLBACK
