/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the three accumulator
 * standard-indirect AR0-indexed and circular WORD16 load families (LD, LD TS,
 * and LD shift16), with distinct MOD5/MOD6/MOD8/MOD10 catalog identities.
 * Each wrapper validates the exact decoded identity and typed mode, stages an
 * indexed or CIRCULAR_WORD16 read, and retains no borrowed pointer. It does not own:
 * catalog/decode definitions, AR resolution or mutation, buses, CPU state,
 * sequencing, timing, observers, allocation, logging, environment, or
 * diagnostics.
 */

#include "op_ld_smem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_ld_smem_common.h"

typedef enum ld_smem_indexed_transform {
    LD_SMEM_INDEXED_SIGNED = 0,
    LD_SMEM_INDEXED_SHIFT16,
    LD_SMEM_INDEXED_TS
} ld_smem_indexed_transform;

static ti_c54x_op_status ld_smem_indexed_validate_sxm(
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

static ti_c54x_op_status ld_smem_indexed_validate_ts_input(
    const ti_c54x_op_input_snapshot *input)
{
    uint8_t shift_bits;
    ti_c54x_op_status status;

    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_smem_indexed_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (!input->t_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    shift_bits = (uint8_t)(input->t_register_raw16 & UINT16_C(0x3F));
    return shift_bits >= UINT8_C(0x20) && shift_bits <= UINT8_C(0x2F)
               ? TI_C54X_OP_STATUS_INVALID_OPERAND
               : TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_smem_indexed_complete_values(
    uint8_t destination,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    uint8_t shift,
    bool arithmetic_right,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_u40_t value_raw40;

    if (input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    {
        const ti_c54x_op_status status = ld_smem_indexed_validate_sxm(input);
        if (status != TI_C54X_OP_STATUS_OK) {
            return status;
        }
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

static ti_c54x_op_status ld_smem_indexed_complete_ts_values(
    uint8_t destination,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    uint8_t shift_bits;
    ti_c54x_op_status status;

    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_smem_indexed_validate_ts_input(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    shift_bits = (uint8_t)(input->t_register_raw16 & UINT16_C(0x3F));
    return ld_smem_indexed_complete_values(
        destination, input, read_value_word,
        shift_bits <= UINT8_C(31)
            ? shift_bits
            : (uint8_t)(UINT8_C(64) - shift_bits),
        shift_bits > UINT8_C(31), next_effect);
}

static ti_c54x_op_status ld_smem_indexed_complete_for_id(
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    uint8_t expected_mode,
    ld_smem_indexed_transform transform,
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
    status = ti_c54x_ld_smem_validate_indexed_instruction(
        instruction, expected_id, expected_resources,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    switch (transform) {
    case LD_SMEM_INDEXED_SIGNED:
        return ld_smem_indexed_complete_values(
            operands.destination, input, read_value_word, UINT8_C(0), false,
            next_effect);
    case LD_SMEM_INDEXED_SHIFT16:
        return ld_smem_indexed_complete_values(
            operands.destination, input, read_value_word, UINT8_C(16), false,
            next_effect);
    case LD_SMEM_INDEXED_TS:
        return ld_smem_indexed_complete_ts_values(
            operands.destination, input, read_value_word, next_effect);
    }
    return TI_C54X_OP_STATUS_INVALID_OPERAND;
}

#define DEFINE_LD_SMEM_INDEXED_CALLBACKS(suffix, id, mode, resources, transform) \
    static ti_c54x_op_status ld_smem_##suffix##_complete(                         \
        const ti_c54x_decoded_instruction *instruction,                          \
        const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,       \
        ti_c54x_op_effect *next_effect)                                          \
    {                                                                              \
        return ld_smem_indexed_complete_for_id(                                   \
            id, resources, mode, transform, instruction, input, read_value_word, \
            next_effect);                                                         \
    }

DEFINE_LD_SMEM_INDEXED_CALLBACKS(
    mod5, TI_C54X_OPCODE_ID_LD_SMEM_MOD5,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
    LD_SMEM_INDEXED_SIGNED)
DEFINE_LD_SMEM_INDEXED_CALLBACKS(
    mod6, TI_C54X_OPCODE_ID_LD_SMEM_MOD6,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
    LD_SMEM_INDEXED_SIGNED)
DEFINE_LD_SMEM_INDEXED_CALLBACKS(
    ts_mod5, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD5,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5,
    TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, LD_SMEM_INDEXED_TS)
DEFINE_LD_SMEM_INDEXED_CALLBACKS(
    ts_mod6, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD6,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6,
    TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, LD_SMEM_INDEXED_TS)
DEFINE_LD_SMEM_INDEXED_CALLBACKS(
    shift16_mod5, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD5,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
    LD_SMEM_INDEXED_SHIFT16)
DEFINE_LD_SMEM_INDEXED_CALLBACKS(
    shift16_mod6, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD6,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
    LD_SMEM_INDEXED_SHIFT16)

#undef DEFINE_LD_SMEM_INDEXED_CALLBACKS

static ti_c54x_op_status ld_smem_indexed_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    uint8_t expected_mode,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_indexed_instruction(
        instruction, expected_id, expected_resources,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (expected_resources == TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER) {
        status = ld_smem_indexed_validate_ts_input(input);
    } else {
        status = ld_smem_indexed_validate_sxm(input);
    }
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_indexed_memory_read(&operands, expected_mode, complete,
                                              effect);
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_smem_indexed_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    uint8_t expected_mode,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || trusted_operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_indexed_trusted_operands(
        instruction, trusted_operands, expected_id, expected_resources,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (expected_resources == TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER) {
        status = ld_smem_indexed_validate_ts_input(input);
        if (status != TI_C54X_OP_STATUS_OK) {
            return status;
        }
    } else {
        status = ld_smem_indexed_validate_sxm(input);
        if (status != TI_C54X_OP_STATUS_OK) {
            return status;
        }
    }
    ti_c54x_ld_smem_stage_indexed_memory_read(&operands, expected_mode, complete,
                                              effect);
    return TI_C54X_OP_STATUS_OK;
}

#define DEFINE_LD_SMEM_INDEXED_WRAPPERS(suffix, id, mode, resources)             \
    ti_c54x_op_status ti_c54x_op_ld_smem_##suffix(                               \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)       \
    {                                                                             \
        return ld_smem_indexed_checked(instruction, input, id, resources, mode,  \
                                       ld_smem_##suffix##_complete, effect);     \
    }                                                                             \
    ti_c54x_op_status ti_c54x_trusted_op_ld_smem_##suffix(                       \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_trusted_operand_values *operands,                         \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)       \
    {                                                                             \
        return ld_smem_indexed_trusted(instruction, operands, input, id,         \
                                       resources, mode,                           \
                                       ld_smem_##suffix##_complete, effect);     \
    }

DEFINE_LD_SMEM_INDEXED_WRAPPERS(
    mod5, TI_C54X_OPCODE_ID_LD_SMEM_MOD5,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, TI_C54X_INPUT_RESOURCES_STATUS_SXM)
DEFINE_LD_SMEM_INDEXED_WRAPPERS(
    mod6, TI_C54X_OPCODE_ID_LD_SMEM_MOD6,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, TI_C54X_INPUT_RESOURCES_STATUS_SXM)
DEFINE_LD_SMEM_INDEXED_WRAPPERS(
    ts_mod5, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD5,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5,
    TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER)
DEFINE_LD_SMEM_INDEXED_WRAPPERS(
    ts_mod6, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD6,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6,
    TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER)
DEFINE_LD_SMEM_INDEXED_WRAPPERS(
    shift16_mod5, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD5,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, TI_C54X_INPUT_RESOURCES_STATUS_SXM)
DEFINE_LD_SMEM_INDEXED_WRAPPERS(
    shift16_mod6, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD6,
    (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, TI_C54X_INPUT_RESOURCES_STATUS_SXM)

#undef DEFINE_LD_SMEM_INDEXED_WRAPPERS
