/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for one-word
 * LDR Smem,dst. Each checked leaf validates its catalog/decode
 * contract through the shared Smem common module, rejects the
 * write-only MOD3 form, and stages a single-indirect WORD16
 * read; its static completion validates the copied instruction
 * and SXM input before staging the rounded accumulator write.
 * The trusted leaves consume constructor-validated operands
 * while retaining SXM input validation.
 * Does not own: opcode inventory, generic decode, AR resolution
 * or mutation, buses, CPU/state mutation, sequencing, timing,
 * observers, allocation, logging, environment, or diagnostics.
 * Borrowed instruction/input/effect objects are used for one
 * call only and no pointer is retained; failed calls leave the
 * caller-owned effect unchanged.
 */

#include "op_ldr.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "op_ld_smem_common.h"

static ti_c54x_op_status ldr_validate_sxm(
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

static ti_c54x_op_status ldr_complete_values(
    uint8_t destination,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_op_status input_status = ldr_validate_sxm(input);
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
    value_raw40 =
        (ti_c54x_u40_logical_left_shift(value_raw40, UINT8_C(16)) +
         UINT64_C(0x8000)) &
        TI_C54X_U40_MASK;
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = value_raw40,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .destination_overflow_action =
                    TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE}}};
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ldr_complete(
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
        instruction, TI_C54X_OPCODE_ID_LDR, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
        &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ldr_complete_values(operands.destination, input, read_value_word,
                               next_effect);
}

ti_c54x_op_status ti_c54x_op_ldr(
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
        instruction, TI_C54X_OPCODE_ID_LDR, TI_C54X_INPUT_RESOURCES_STATUS_SXM,
        &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ldr_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_memory_read(&operands, ldr_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ldr(
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
    status = ldr_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_memory_read(&decoded_operands, ldr_complete,
                                      effect);
    return TI_C54X_OP_STATUS_OK;
}
