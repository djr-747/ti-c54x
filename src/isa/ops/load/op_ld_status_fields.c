/*
 * Module contract
 *
 * Owns: checked, pure construction of immediate status-field write effects
 * for DP, ASM, and ARP. It copies only the raw field value and target into a
 * caller-owned effect after complete descriptor validation.
 * Does not own: opcode inventory, decode classification, CPU/state mutation,
 * status commit, PC sequencing, repeat policy, timing, memory, buses,
 * diagnostics, or callbacks.
 */

#include "op_ld_status_fields.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"

static ti_c54x_op_status
ti_c54x_op_ld_status_field_common(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id,
    uint8_t expected_width,
    ti_c54x_op_status_field_target target)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value field = {0U, 0U};
    ti_c54x_op_effect staged = {0};

    if (instruction == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_mask == UINT16_C(0) ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout <= TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        metadata->operand_layout >= TI_C54X_OPERAND_LAYOUT_COUNT ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) !=
            metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count, &view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    {
        const ti_c54x_program_words_view input = {
            instruction->bundle.program_words, metadata->program_word_count};
        const ti_c54x_operand_layout_status status =
            ti_c54x_operand_layout_set_extract_operand(
                &view, &input, UINT8_C(0), &field);
        if (status != TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != expected_width ||
            field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
    }

    staged.kind = TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE;
    staged.payload.status_field_write.target = target;
    staged.payload.status_field_write.raw_value = (uint16_t)field.raw_value;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status
ti_c54x_op_ld_immediate_dp(const ti_c54x_decoded_instruction *instruction,
                           ti_c54x_op_effect *effect)
{
    return ti_c54x_op_ld_status_field_common(
        instruction, effect, TI_C54X_OPCODE_ID_LD_IMMEDIATE_DP,
        UINT8_C(9), TI_C54X_OP_STATUS_FIELD_TARGET_DP);
}

ti_c54x_op_status
ti_c54x_op_ld_immediate_asm(const ti_c54x_decoded_instruction *instruction,
                            ti_c54x_op_effect *effect)
{
    return ti_c54x_op_ld_status_field_common(
        instruction, effect, TI_C54X_OPCODE_ID_LD_IMMEDIATE_ASM,
        UINT8_C(5), TI_C54X_OP_STATUS_FIELD_TARGET_ASM);
}

ti_c54x_op_status
ti_c54x_op_ld_immediate_arp(const ti_c54x_decoded_instruction *instruction,
                            ti_c54x_op_effect *effect)
{
    return ti_c54x_op_ld_status_field_common(
        instruction, effect, TI_C54X_OPCODE_ID_LD_IMMEDIATE_ARP,
        UINT8_C(3), TI_C54X_OP_STATUS_FIELD_TARGET_ARP);
}
