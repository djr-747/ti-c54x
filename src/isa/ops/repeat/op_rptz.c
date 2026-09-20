/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for RPTZ dst,#lk
 * (D-selected accumulator zero plus RC = lk through one
 * REPEAT_START effect). The count is a raw 16-bit value with no
 * sign extension and no input requirements. Does not own: opcode
 * inventory, operand-layout definitions, RC or accumulator
 * state, buses, CPU/state mutation, sequencing, timing,
 * observers, allocation, logging, environment, or diagnostics.
 */

#include "op_rptz.h"

#include <stddef.h>
#include <stdint.h>

#include "../../decode/operand_field.h"
#include "../../decode/operand_layout.h"

typedef struct rptz_operands {
    uint8_t destination;
    uint16_t count;
} rptz_operands;

static ti_c54x_op_status rptz_validate_metadata(
    const ti_c54x_decoded_instruction *instruction)
{
    const ti_c54x_opcode_metadata *metadata;

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RPTZ_DST_LK);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_RPTZ_DST_LK ||
        metadata->first_word_value != UINT16_C(0xF071) ||
        metadata->first_word_mask != UINT16_C(0xFEFF) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->id != TI_C54X_OPCODE_ID_RPTZ_DST_LK ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFEFF)) != UINT16_C(0xF071)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status rptz_extract(
    const ti_c54x_decoded_instruction *instruction, rptz_operands *operands)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value count_field = {0U, 0U};
    ti_c54x_op_status status;

    if (instruction == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = rptz_validate_metadata(instruction);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16,
            UINT8_C(2), &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &destination_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &count_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            destination_field.width_bits != UINT8_C(1) ||
            destination_field.raw_value > UINT64_C(1) ||
            count_field.width_bits != UINT8_C(16) ||
            count_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    operands->destination = (uint8_t)destination_field.raw_value;
    operands->count = (uint16_t)count_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status rptz_stage(uint8_t destination, uint16_t count,
                                    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    staged.kind = TI_C54X_OP_EFFECT_REPEAT_START;
    staged.payload.repeat_start.operand = count;
    staged.payload.repeat_start.zero_destination = true;
    staged.payload.repeat_start.zero_destination_index = destination;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_rptz(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    rptz_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = rptz_extract(instruction, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return rptz_stage(operands.destination, operands.count, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_rptz(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(16) ||
        operands->values[1].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return rptz_stage((uint8_t)operands->values[0].raw_value,
                      (uint16_t)operands->values[1].raw_value, effect);
}
