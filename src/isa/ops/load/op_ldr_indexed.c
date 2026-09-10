/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the LDR
 * standard-indirect AR0-indexed WORD16 load family, with distinct MOD5/MOD6
 * catalog identities. Each wrapper validates the exact decoded identity and
 * typed mode, stages an indexed WORD16 read, and completes with the
 * sibling-mirrored round adder ((SXM-widened Smem << 16) + 0x8000) while
 * preserving C/TC/OV with no OVM input. It retains no borrowed pointer.
 * Does not own: catalog/decode definitions, AR resolution or mutation,
 * buses, CPU state, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_ldr.h"

#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_ld_smem_common.h"

static ti_c54x_op_status ldr_indexed_validate_sxm(
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

static ti_c54x_op_status ldr_indexed_complete_values(
    uint8_t destination,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_op_status input_status;
    ti_c54x_u40_t value_raw40;

    if (input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    input_status = ldr_indexed_validate_sxm(input);
    if (input_status != TI_C54X_OP_STATUS_OK) {
        return input_status;
    }
    value_raw40 = ti_c54x_u40_from_unsigned(read_value_word);
    if (input->status_sxm != UINT8_C(0) &&
        (read_value_word & UINT16_C(0x8000)) != UINT16_C(0)) {
        value_raw40 = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                      (uint64_t)read_value_word;
    }
    value_raw40 =
        (ti_c54x_u40_logical_left_shift(value_raw40, UINT8_C(16)) +
         UINT64_C(0x8000)) &
        TI_C54X_U40_MASK;
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = value_raw40,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .destination_overflow_action =
                    TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE}}};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ldr_indexed_complete_for_id(
    ti_c54x_opcode_id expected_id,
    uint8_t expected_mode,
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
        instruction, expected_id, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ldr_indexed_complete_values(operands.destination, input,
                                       read_value_word, next_effect);
}

#define DEFINE_LDR_INDEXED_CALLBACKS(suffix, id, mode)                         \
    static ti_c54x_op_status ldr_##suffix##_complete(                           \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,      \
        ti_c54x_op_effect *next_effect)                                         \
    {                                                                             \
        return ldr_indexed_complete_for_id(id, mode, instruction, input,        \
                                           read_value_word, next_effect);       \
    }

DEFINE_LDR_INDEXED_CALLBACKS(mod5, TI_C54X_OPCODE_ID_LDR_MOD5,
                             (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5)
DEFINE_LDR_INDEXED_CALLBACKS(mod6, TI_C54X_OPCODE_ID_LDR_MOD6,
                             (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6)

#undef DEFINE_LDR_INDEXED_CALLBACKS

static ti_c54x_op_status ldr_indexed_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id,
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
        instruction, expected_id, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ldr_indexed_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_indexed_memory_read(&operands, expected_mode,
                                              complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ldr_indexed_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id,
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
        instruction, trusted_operands, expected_id,
        TI_C54X_INPUT_RESOURCES_STATUS_SXM,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ldr_indexed_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_indexed_memory_read(&operands, expected_mode,
                                              complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

#define DEFINE_LDR_INDEXED_WRAPPERS(suffix, id, mode)                          \
    ti_c54x_op_status ti_c54x_op_ldr_##suffix(                                  \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)      \
    {                                                                            \
        return ldr_indexed_checked(instruction, input, id, mode,                \
                                   ldr_##suffix##_complete, effect);            \
    }                                                                            \
    ti_c54x_op_status ti_c54x_trusted_op_ldr_##suffix(                          \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_trusted_operand_values *operands,                         \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)      \
    {                                                                            \
        return ldr_indexed_trusted(instruction, operands, input, id, mode,      \
                                   ldr_##suffix##_complete, effect);            \
    }

DEFINE_LDR_INDEXED_WRAPPERS(mod5, TI_C54X_OPCODE_ID_LDR_MOD5,
                            (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5)
DEFINE_LDR_INDEXED_WRAPPERS(mod6, TI_C54X_OPCODE_ID_LDR_MOD6,
                            (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6)

#undef DEFINE_LDR_INDEXED_WRAPPERS
