/*
 * Module contract
 *
 * Owns: checked and trusted pure SSBX/RSBX semantic leaves. Each successful
 * call stages one copied status-register/bit/value effect and commits it to
 * the caller's effect only after catalog and operand validation succeeds.
 * Does not own: status-register mutation, decode lookup, CPU sequencing,
 * timing, I/O, allocation, environment, or diagnostics.
 */

#include "op_status_bits.h"

#include <stdbool.h>
#include <stddef.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static bool metadata_is_valid(const ti_c54x_opcode_metadata *metadata,
                              ti_c54x_opcode_id id)
{
    return metadata != NULL && metadata->id == id &&
           metadata->first_word_mask != UINT16_C(0) &&
           (metadata->first_word_value &
            (uint16_t)~metadata->first_word_mask) == UINT16_C(0) &&
           metadata->program_word_count == UINT8_C(1) &&
           metadata->operand_layout == TI_C54X_OPERAND_LAYOUT_STATUS_BIT &&
           metadata->repeatability == TI_C54X_REPEATABILITY_INELIGIBLE &&
           metadata->input_resources == TI_C54X_INPUT_RESOURCES_NONE;
}

static ti_c54x_op_status emit_status_bit(
    uint8_t n, uint8_t sbit, uint8_t value, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (n > UINT8_C(1) || sbit > UINT8_C(15) || value > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged.kind = TI_C54X_OP_EFFECT_STATUS_BIT_WRITE;
    staged.payload.status_bit_write.target_register =
        n == UINT8_C(0) ? TI_C54X_OP_STATUS_REGISTER_ST0
                         : TI_C54X_OP_STATUS_REGISTER_ST1;
    staged.payload.status_bit_write.bit_index = sbit;
    staged.payload.status_bit_write.raw_value = value;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status checked_status_bit(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id id, uint8_t value)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value n_field = {0U, 0U};
    ti_c54x_operand_field_value sbit_field = {0U, 0U};
    ti_c54x_op_effect staged;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(id);
    if (!metadata_is_valid(metadata, id) || instruction->id != id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
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
                &layout_view, &words, UINT8_C(0), &n_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &sbit_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            n_field.width_bits != UINT8_C(1) ||
            sbit_field.width_bits != UINT8_C(4) || n_field.raw_value > 1U ||
            sbit_field.raw_value > 15U) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    staged = (ti_c54x_op_effect){0};
    if (emit_status_bit((uint8_t)n_field.raw_value,
                        (uint8_t)sbit_field.raw_value, value, &staged) !=
        TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status trusted_status_bit(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    uint8_t value)
{
    ti_c54x_op_effect staged;

    (void)instruction;
    (void)input;
    if (operands == NULL || effect == NULL || operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[1].width_bits != UINT8_C(4)) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    staged = (ti_c54x_op_effect){0};
    if (emit_status_bit((uint8_t)operands->values[0].raw_value,
                        (uint8_t)operands->values[1].raw_value, value,
                        &staged) != TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_ssbx(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return checked_status_bit(instruction, input, effect,
                              TI_C54X_OPCODE_ID_SSBX_STATUS, UINT8_C(1));
}

ti_c54x_op_status ti_c54x_op_rsbx(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return checked_status_bit(instruction, input, effect,
                              TI_C54X_OPCODE_ID_RSBX_STATUS, UINT8_C(0));
}

ti_c54x_op_status ti_c54x_op_ssbx_status(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return ti_c54x_op_ssbx(instruction, input, effect);
}

ti_c54x_op_status ti_c54x_op_rsbx_status(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return ti_c54x_op_rsbx(instruction, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_ssbx(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return trusted_status_bit(instruction, operands, input, effect,
                              UINT8_C(1));
}

ti_c54x_op_status ti_c54x_trusted_op_rsbx(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return trusted_status_bit(instruction, operands, input, effect,
                              UINT8_C(0));
}

ti_c54x_op_status ti_c54x_trusted_op_ssbx_status(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return ti_c54x_trusted_op_ssbx(instruction, operands, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_rsbx_status(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return ti_c54x_trusted_op_rsbx(instruction, operands, input, effect);
}
