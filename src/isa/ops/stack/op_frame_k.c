/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for FRAME K.
 * Does not own: opcode inventory, operand-layout definitions,
 * SP state, buses, CPU or architectural state, sequencing,
 * timing, observers, diagnostics, allocation, logging,
 * environment, or callbacks.
 */

#include "op_frame_k.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../../decode/operand_layout.h"
#include "../../decode/operand_field.h"

static ti_c54x_op_status frame_k_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_operand_field_value *operand)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;

    if (instruction == NULL || operand == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FRAME);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_FRAME ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_STACK_POINTER ||
        instruction->id != TI_C54X_OPCODE_ID_FRAME ||
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
            field.width_bits != UINT8_C(8) ||
            field.raw_value > UINT64_C(0xFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *operand = field;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status frame_k_stage(
    uint8_t suffix, const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    /* K is an 8-bit two's-complement offset; SP arithmetic wraps
     * mod 65536 (project-interpretation pin: the manual states no
     * stack-overflow status and Status Bits None). */
    int16_t offset = (int16_t)(int8_t)suffix;
    int32_t sum;
    ti_c54x_op_effect staged;

    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!input->stack_pointer_known) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    sum = (int32_t)input->stack_pointer_raw16 + (int32_t)offset;
    memset(&staged, 0, sizeof(staged));
    staged.kind = TI_C54X_OP_EFFECT_SP_WRITE;
    staged.payload.sp_write.value_after =
        (uint16_t)(sum & (int32_t)UINT16_C(0xFFFF));
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_frame_k(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_operand_field_value operand;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (frame_k_validate_instruction(instruction, &operand) !=
        TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return frame_k_stage((uint8_t)operand.raw_value, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_frame_k(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(8) ||
        operands->values[0].raw_value > UINT64_C(0xFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return frame_k_stage((uint8_t)operands->values[0].raw_value, input,
                         effect);
}
