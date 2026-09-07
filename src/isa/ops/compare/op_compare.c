/* Module contract
 * Owns: pure signed full-width MAX/MIN selection over copied raw40 inputs.
 * Does not own: decode storage, architectural mutation, sequencing, or CPU lifecycle.
 */

#include "op_compare.h"

#include <stddef.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status select_values(
    uint8_t destination,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect,
    bool choose_max)
{
    ti_c54x_op_effect staged = {0};
    int64_t a_signed;
    int64_t b_signed;
    bool choose_a;

    if (!input->accumulator_a_known || !input->accumulator_b_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK ||
        input->accumulator_b_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    a_signed = ti_c54x_u40_sign_extend(input->accumulator_a_raw40);
    b_signed = ti_c54x_u40_sign_extend(input->accumulator_b_raw40);
    choose_a = choose_max ? a_signed >= b_signed : a_signed <= b_signed;
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = destination;
    staged.payload.accumulator_write.value_raw40 =
        ti_c54x_u40_mask(choose_a ? input->accumulator_a_raw40
                                  : input->accumulator_b_raw40);
    staged.payload.accumulator_write.status_delta.c_action =
        choose_a ? TI_C54X_OP_STATUS_BIT_WRITE_0
                 : TI_C54X_OP_STATUS_BIT_WRITE_1;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status select_common(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id,
    bool choose_max)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value & (uint16_t)~metadata->first_word_mask) !=
            UINT16_C(0) ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout <= TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        metadata->operand_layout >= TI_C54X_OPERAND_LAYOUT_COUNT ||
        metadata->repeatability < TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_ACCUMULATORS ||
        instruction->id != expected_id ||
        instruction->operand_layout != TI_C54X_OPERAND_LAYOUT_MAX_MIN_ACC ||
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
                &layout_view, &words, UINT8_C(0), &destination_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            destination_field.width_bits != UINT8_C(1) ||
            destination_field.raw_value > UINT64_C(1)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    return select_values((uint8_t)destination_field.raw_value, input, effect,
                         choose_max);
}

ti_c54x_op_status ti_c54x_op_max_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return select_common(instruction, input, effect,
                         TI_C54X_OPCODE_ID_MAX_ACC, true);
}

ti_c54x_op_status ti_c54x_op_min_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return select_common(instruction, input, effect,
                         TI_C54X_OPCODE_ID_MIN_ACC, false);
}

ti_c54x_op_status ti_c54x_trusted_op_max_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;
    return select_values((uint8_t)operands->values[0].raw_value,
                         input, effect, true);
}

ti_c54x_op_status ti_c54x_trusted_op_min_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;
    return select_values((uint8_t)operands->values[0].raw_value,
                         input, effect, false);
}
