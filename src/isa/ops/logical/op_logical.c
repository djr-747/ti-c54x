/* Module contract
 * Owns: pure AND, OR, and XOR accumulator semantics over copied canonical
 * raw40 inputs, including source/destination selection and signed five-bit
 * full-width shifts.
 * Does not own: catalog inventory, decode tables, accumulator storage, status
 * mutation, sequencing, CPU lifecycle, timing, memory, or diagnostics.
 */

#include "op_logical.h"

#include <stddef.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"

typedef enum ti_c54x_logical_kind {
    TI_C54X_LOGICAL_AND = 0,
    TI_C54X_LOGICAL_OR = 1,
    TI_C54X_LOGICAL_XOR = 2
} ti_c54x_logical_kind;

static ti_c54x_op_status logical_common(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id,
    uint16_t expected_first_word,
    ti_c54x_logical_kind kind)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value shift_field = {0U, 0U};
    ti_c54x_op_effect staged = {0};
    ti_c54x_u40_t source_raw40;
    ti_c54x_u40_t destination_raw40;
    uint8_t source;
    uint8_t destination;
    int shift;
    uint64_t shifted_source;
    uint64_t result;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_value != expected_first_word ||
        metadata->first_word_mask != UINT16_C(0xFCE0) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_ACCUMULATORS ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        instruction->bundle.program_word_count != metadata->program_word_count ||
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
                &layout_view, &words, UINT8_C(0), &source_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &destination_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &shift_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            source_field.width_bits != UINT8_C(1) ||
            destination_field.width_bits != UINT8_C(1) ||
            shift_field.width_bits != UINT8_C(5) ||
            source_field.raw_value > UINT64_C(1) ||
            destination_field.raw_value > UINT64_C(1) ||
            shift_field.raw_value > UINT64_C(0x1F)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    source = (uint8_t)source_field.raw_value;
    destination = (uint8_t)destination_field.raw_value;
    if (source == UINT8_C(0)) {
        source_raw40 = input->accumulator_a_raw40;
        if (!input->accumulator_a_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
    } else {
        source_raw40 = input->accumulator_b_raw40;
        if (!input->accumulator_b_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
    }

    if (destination == UINT8_C(0)) {
        destination_raw40 = input->accumulator_a_raw40;
        if (source != destination && !input->accumulator_a_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
    } else {
        destination_raw40 = input->accumulator_b_raw40;
        if (source != destination && !input->accumulator_b_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
    }
    if (source_raw40 > TI_C54X_U40_MASK ||
        (source != destination && destination_raw40 > TI_C54X_U40_MASK)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    shift = (int)shift_field.raw_value;
    if ((shift & 0x10) != 0) {
        shift -= 32;
    }
    if (shift < 0) {
        shifted_source = source_raw40 >> (unsigned)(-shift);
    } else if (shift == 0) {
        shifted_source = source_raw40;
    } else {
        shifted_source =
            (source_raw40 << (unsigned)shift) & TI_C54X_U40_MASK;
    }

    switch (kind) {
    case TI_C54X_LOGICAL_AND:
        result = destination_raw40 & shifted_source;
        break;
    case TI_C54X_LOGICAL_OR:
        result = destination_raw40 | shifted_source;
        break;
    case TI_C54X_LOGICAL_XOR:
        result = destination_raw40 ^ shifted_source;
        break;
    default:
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = destination;
    staged.payload.accumulator_write.value_raw40 = ti_c54x_u40_mask(result);
    staged.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_and_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return logical_common(instruction, input, effect,
                          TI_C54X_OPCODE_ID_AND_ACC_SHIFT,
                          UINT16_C(0xF080), TI_C54X_LOGICAL_AND);
}

ti_c54x_op_status ti_c54x_op_or_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return logical_common(instruction, input, effect,
                          TI_C54X_OPCODE_ID_OR_ACC_SHIFT,
                          UINT16_C(0xF0A0), TI_C54X_LOGICAL_OR);
}

ti_c54x_op_status ti_c54x_op_xor_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return logical_common(instruction, input, effect,
                          TI_C54X_OPCODE_ID_XOR_ACC_SHIFT,
                          UINT16_C(0xF0C0), TI_C54X_LOGICAL_XOR);
}
