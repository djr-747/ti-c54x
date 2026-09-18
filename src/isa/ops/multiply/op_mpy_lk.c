/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for MPY #lk, dst
 * (syntax 4, UNROUNDED form only): (T) times signed lk16 stored
 * into the destination accumulator through the shared multiply
 * core with STORE select, FRCT shift, and OVM saturation plus
 * sticky OVdst. The product overwrites the destination
 * accumulator, so destination knownness is not required; only T,
 * FRCT, and OVM gate execution. T is READ and never written; C
 * and TC are preserved. The R (rounding) suffix form defers as
 * INVALID_DECODE.
 * Does not own: opcode inventory, operand-layout definitions,
 * T/accumulator/status mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_mpy_lk.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_multiply.h"

#define MPY_LK_FIRST_WORD_BASE UINT16_C(0xF066)
#define MPY_LK_FIRST_WORD_MASK UINT16_C(0xFEFF)

typedef struct mpy_lk_fields {
    uint8_t destination;
    uint16_t immediate;
} mpy_lk_fields;

static ti_c54x_op_status mpy_lk_extract(
    const ti_c54x_decoded_instruction *instruction,
    mpy_lk_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value immediate_field = {0U, 0U};
    ti_c54x_program_words_view words;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16 ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   MPY_LK_FIRST_WORD_MASK) != MPY_LK_FIRST_WORD_BASE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16,
            UINT8_C(2),
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    if (ti_c54x_operand_layout_set_extract_operand(
            &layout_view, &words, UINT8_C(0),
            &destination_field) != TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
        ti_c54x_operand_layout_set_extract_operand(
            &layout_view, &words, UINT8_C(1),
            &immediate_field) != TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
        destination_field.width_bits != UINT8_C(1) ||
        destination_field.raw_value > UINT64_C(1) ||
        immediate_field.width_bits != UINT8_C(16) ||
        immediate_field.raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    fields->destination = (uint8_t)destination_field.raw_value;
    fields->immediate = (uint16_t)immediate_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mpy_lk_validate_inputs(
    const ti_c54x_op_input_snapshot *input)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!input->t_register_known || !input->status_frct_known ||
        !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_frct > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mpy_lk_values(
    const mpy_lk_fields *fields,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_multiply_operands operands;
    ti_c54x_multiply_result result;
    ti_c54x_multiply_status multiply_status;
    ti_c54x_op_effect staged;
    ti_c54x_op_status status;

    if (fields == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mpy_lk_validate_inputs(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    operands.t_raw16 = input->t_register_raw16;
    operands.t_known = true;
    operands.multiplicand_raw16 = fields->immediate;
    operands.multiplicand_known = true;
    operands.accumulator_raw40 = UINT64_C(0);
    operands.accumulator_known = false;
    operands.frct = input->status_frct;
    operands.frct_known = true;
    operands.ovm = input->status_ovm;
    operands.ovm_known = true;
    operands.rounding = false;
    operands.unsigned_operands = false;
    operands.unsigned_t_only = false;
    operands.select = TI_C54X_MULTIPLY_STORE;
    multiply_status = ti_c54x_multiply_accumulate(&operands, &result);
    if (multiply_status == TI_C54X_MULTIPLY_UNKNOWN) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (multiply_status != TI_C54X_MULTIPLY_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields->destination;
    staged.payload.accumulator_write.value_raw40 = result.accumulator_raw40;
    staged.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.destination_overflow_action =
        result.overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                        : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_mpy_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    mpy_lk_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mpy_lk_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return mpy_lk_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_mpy_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    mpy_lk_fields fields;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(16) ||
        operands->values[1].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields.destination = (uint8_t)operands->values[0].raw_value;
    fields.immediate = (uint16_t)operands->values[1].raw_value;
    return mpy_lk_values(&fields, input, effect);
}
