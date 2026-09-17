/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for SQUR A,dst
 * (TI SPRU172C syntax 2, opcode 1111 010D 1000 1101) across the
 * D destination select. The leaves complete (A(32-16)) squared
 * into the selected accumulator through the multiply core with
 * STORE select, FRCT/OVM handling, and no rounding; T is never
 * touched and C/TC ride through unchanged.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include "op_squr_adst.h"

#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_multiply.h"

static ti_c54x_op_status squr_adst_complete(
    uint8_t destination, const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_multiply_operands operands;
    ti_c54x_multiply_result result;
    ti_c54x_multiply_status multiply_status;
    ti_c54x_op_effect staged = {0};
    uint16_t a_high;

    if (!input->accumulator_a_known || !input->status_frct_known ||
        !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (input->status_frct > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    /* Square: A-high feeds both multiplier inputs; T is untouched. */
    a_high =
        (uint16_t)((input->accumulator_a_raw40 >> 16) & UINT64_C(0xFFFF));
    operands.t_raw16 = a_high;
    operands.t_known = true;
    operands.multiplicand_raw16 = a_high;
    operands.multiplicand_known = true;
    operands.accumulator_raw40 = UINT64_C(0);
    operands.accumulator_known = false;
    operands.frct = input->status_frct;
    operands.frct_known = true;
    operands.ovm = input->status_ovm;
    operands.ovm_known = true;
    operands.rounding = false;
    operands.select = TI_C54X_MULTIPLY_STORE;
    multiply_status = ti_c54x_multiply_accumulate(&operands, &result);
    if (multiply_status == TI_C54X_MULTIPLY_UNKNOWN) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (multiply_status != TI_C54X_MULTIPLY_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = destination;
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

static ti_c54x_op_status squr_adst_extract(
    const ti_c54x_decoded_instruction *instruction, uint8_t *destination)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field = {0U, 0U};
    ti_c54x_program_words_view words;

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_SQUR_ADST);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_SQUR_ADST ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_FRCT_OVM ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_MAX_MIN_ACC ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->repeatability != TI_C54X_REPEATABILITY_ELIGIBLE ||
        instruction->id != TI_C54X_OPCODE_ID_SQUR_ADST ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value ||
        metadata->first_word_value != UINT16_C(0xF48D) ||
        metadata->first_word_mask != UINT16_C(0xFEFF)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    if (ti_c54x_operand_layout_set_extract_operand(
            &layout_view, &words, UINT8_C(0),
            &field) != TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
        field.width_bits != UINT8_C(1) ||
        field.raw_value > UINT64_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    *destination = (uint8_t)field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_squr_adst(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t destination;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = squr_adst_extract(instruction, &destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return squr_adst_complete(destination, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_squr_adst(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return squr_adst_complete((uint8_t)operands->values[0].raw_value, input,
                              effect);
}
