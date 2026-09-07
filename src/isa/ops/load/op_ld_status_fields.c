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
#include "isa/ops/dispatch/trusted_semantics.h"

static void ld_status_field_write(
    uint16_t raw_value,
    ti_c54x_op_status_field_target target,
    ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE,
        .payload.status_field_write = {.target = target, .raw_value = raw_value}};
}

static ti_c54x_op_status ld_status_field_value(
    uint16_t raw_value,
    ti_c54x_op_status_field_target target,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    ld_status_field_write(raw_value, target, &staged);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status
ti_c54x_op_ld_status_field_common(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id,
    uint8_t expected_width,
    ti_c54x_op_status_field_target target)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value field = {0U, 0U};

    if (instruction == NULL || input == NULL || effect == NULL) {
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
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
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
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words, metadata->program_word_count};
        const ti_c54x_operand_layout_status status =
            ti_c54x_operand_layout_set_extract_operand(
                &view, &words, UINT8_C(0), &field);
        if (status != TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != expected_width ||
            field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
    }

    return ld_status_field_value((uint16_t)field.raw_value, target, effect);
}

ti_c54x_op_status
ti_c54x_op_ld_immediate_dp(const ti_c54x_decoded_instruction *instruction,
                           const ti_c54x_op_input_snapshot *input,
                           ti_c54x_op_effect *effect)
{
    return ti_c54x_op_ld_status_field_common(
        instruction, input, effect, TI_C54X_OPCODE_ID_LD_IMMEDIATE_DP,
        UINT8_C(9), TI_C54X_OP_STATUS_FIELD_TARGET_DP);
}

ti_c54x_op_status
ti_c54x_op_ld_immediate_asm(const ti_c54x_decoded_instruction *instruction,
                            const ti_c54x_op_input_snapshot *input,
                            ti_c54x_op_effect *effect)
{
    return ti_c54x_op_ld_status_field_common(
        instruction, input, effect, TI_C54X_OPCODE_ID_LD_IMMEDIATE_ASM,
        UINT8_C(5), TI_C54X_OP_STATUS_FIELD_TARGET_ASM);
}

ti_c54x_op_status
ti_c54x_op_ld_immediate_arp(const ti_c54x_decoded_instruction *instruction,
                            const ti_c54x_op_input_snapshot *input,
                            ti_c54x_op_effect *effect)
{
    return ti_c54x_op_ld_status_field_common(
        instruction, input, effect, TI_C54X_OPCODE_ID_LD_IMMEDIATE_ARP,
        UINT8_C(3), TI_C54X_OP_STATUS_FIELD_TARGET_ARP);
}

#define TI_C54X_DEFINE_TRUSTED_LD(name, target) \
    ti_c54x_op_status ti_c54x_trusted_op_##name( \
        const ti_c54x_decoded_instruction *instruction, \
        const ti_c54x_trusted_operand_values *operands, \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect) \
    { \
        (void)instruction; \
        (void)input; \
        ld_status_field_write( \
            (uint16_t)operands->values[0].raw_value, target, effect); \
        return TI_C54X_OP_STATUS_OK; \
    }

TI_C54X_DEFINE_TRUSTED_LD(ld_immediate_dp,
                          TI_C54X_OP_STATUS_FIELD_TARGET_DP)
TI_C54X_DEFINE_TRUSTED_LD(ld_immediate_asm,
                          TI_C54X_OP_STATUS_FIELD_TARGET_ASM)
TI_C54X_DEFINE_TRUSTED_LD(ld_immediate_arp,
                          TI_C54X_OP_STATUS_FIELD_TARGET_ARP)

#undef TI_C54X_DEFINE_TRUSTED_LD
