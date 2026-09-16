/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for RPT #lk.
 * Does not own: opcode inventory, operand-layout definitions,
 * RC state, buses, CPU or architectural state, sequencing,
 * timing, observers, diagnostics, allocation, logging,
 * environment, or callbacks.
 */

#include "op_rpt_lk.h"

#include <stddef.h>
#include <stdint.h>

#include "../../decode/operand_layout.h"
#include "../../decode/operand_field.h"
#include "op_rpt_immediate.h"

static ti_c54x_op_status rpt_lk_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_operand_field_value *operand)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;

    if (instruction == NULL || operand == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RPT_LK);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_RPT_LK ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->id != TI_C54X_OPCODE_ID_RPT_LK ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};

        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != UINT8_C(16) ||
            field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *operand = field;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_rpt_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_operand_field_value operand;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = rpt_lk_validate_instruction(instruction, &operand);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ti_c54x_op_rpt_immediate_from_field(&operand, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_rpt_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(16) ||
        operands->values[0].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return ti_c54x_op_rpt_immediate_from_field(&operands->values[0],
                                              effect);
}
