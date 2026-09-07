/* Module contract
 * Owns: pure SFTL accumulator semantics over copied canonical raw40 inputs,
 * including source/destination selection, signed five-bit shifts, and staged
 * accumulator/C effects.
 * Does not own: accumulator storage, status mutation, decode tables,
 * sequencing, CPU lifecycle, timing, repeat/interrupt policy, or diagnostics.
 */

#include "op_shift.h"

#include <stddef.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status sftl_values(
    uint8_t source,
    uint8_t destination,
    uint8_t raw_shift,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};
    ti_c54x_u40_t source_raw40;
    uint64_t low32;
    uint64_t shifted;
    uint8_t c_value;
    int shift;

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
    if (source_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    shift = (int)raw_shift;
    if ((shift & 0x10) != 0) {
        shift -= 32;
    }
    low32 = source_raw40 & UINT64_C(0xFFFFFFFF);
    if (shift < 0) {
        const unsigned count = (unsigned)(-shift);
        shifted = low32 >> count;
        c_value = (uint8_t)((low32 >> (count - 1U)) & UINT64_C(1));
    } else if (shift == 0) {
        shifted = low32;
        c_value = UINT8_C(0);
    } else {
        const unsigned count = (unsigned)shift;
        shifted = (low32 << count) & UINT64_C(0xFFFFFFFF);
        c_value = (uint8_t)((low32 >> (32U - count)) & UINT64_C(1));
    }

    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = destination;
    staged.payload.accumulator_write.value_raw40 = ti_c54x_u40_mask(shifted);
    staged.payload.accumulator_write.status_delta.c_action =
        c_value != UINT8_C(0) ? TI_C54X_OP_STATUS_BIT_WRITE_1
                              : TI_C54X_OP_STATUS_BIT_WRITE_0;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_sftl_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value shift_field = {0U, 0U};

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_SFTL_ACC);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_SFTL_ACC ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value & (uint16_t)~metadata->first_word_mask) !=
            UINT16_C(0) ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout <= TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        metadata->operand_layout >= TI_C54X_OPERAND_LAYOUT_COUNT ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5 ||
        metadata->repeatability < TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_ACCUMULATORS ||
        instruction->id != TI_C54X_OPCODE_ID_SFTL_ACC ||
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

    return sftl_values((uint8_t)source_field.raw_value,
                       (uint8_t)destination_field.raw_value,
                       (uint8_t)shift_field.raw_value, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_sftl_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;
    return sftl_values((uint8_t)operands->values[0].raw_value,
                       (uint8_t)operands->values[1].raw_value,
                       (uint8_t)operands->values[2].raw_value,
                       input, effect);
}
