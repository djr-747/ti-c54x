/*
 * Module contract
 *
 * Owns: checked/trusted validation and typed READ staging for reverse-carry
 * WORD16 load wrappers. Does not own: opcode-family transforms, decode tables,
 * address resolution, CPU state, buses, sequencing, or diagnostics.
 */

#include "op_ld_smem_reverse_carry_common.h"

#include <stdbool.h>
#include <stddef.h>

static bool has_destination(ti_c54x_operand_layout_id layout)
{
    return layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4;
}

static bool valid_layout(ti_c54x_operand_layout_id layout)
{
    return has_destination(layout) || layout == TI_C54X_OPERAND_LAYOUT_ARF_MOD4;
}

static ti_c54x_op_status validate_shape(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id, ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_indirect_reverse_carry_mode expected_mode,
    ti_c54x_ld_smem_operands *operands)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value destination = {0U, 0U};
    ti_c54x_operand_field_value arf = {0U, 0U};
    ti_c54x_operand_field_value mode = {0U, 0U};
    const bool destination_present = has_destination(expected_layout);

    if (instruction == NULL || operands == NULL || !valid_layout(expected_layout) ||
        (expected_mode != TI_C54X_INDIRECT_REVERSE_CARRY_MOD4 &&
         expected_mode != TI_C54X_INDIRECT_REVERSE_CARRY_MOD7)) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_mask == 0U ||
        (metadata->first_word_value & (uint16_t)~metadata->first_word_mask) != 0U ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->program_word_count > TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout != expected_layout ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != expected_resources || instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] & metadata->first_word_mask) !=
            metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            expected_layout, metadata->program_word_count, &view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words, instruction->bundle.program_word_count};
        const uint8_t arf_index = destination_present ? UINT8_C(1) : UINT8_C(0);
        const uint8_t mode_index = destination_present ? UINT8_C(2) : UINT8_C(1);
        if ((destination_present &&
             ti_c54x_operand_layout_set_extract_operand(
                 &view, &words, UINT8_C(0), &destination) !=
                 TI_C54X_OPERAND_LAYOUT_STATUS_OK) ||
            ti_c54x_operand_layout_set_extract_operand(
                &view, &words, arf_index, &arf) != TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &view, &words, mode_index, &mode) != TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            (destination_present &&
             (destination.width_bits != UINT8_C(1) || destination.raw_value > 1U)) ||
            arf.width_bits != UINT8_C(3) || arf.raw_value > UINT64_C(7) ||
            arf.raw_value == UINT64_C(0) || mode.width_bits != UINT8_C(4) ||
            mode.raw_value != expected_mode) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    operands->destination = destination_present ? (uint8_t)destination.raw_value : 0U;
    operands->arf = (uint8_t)arf.raw_value;
    operands->mode = (uint8_t)mode.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_ld_smem_validate_reverse_instruction(
    const ti_c54x_decoded_instruction *instruction, ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_indirect_reverse_carry_mode expected_mode,
    ti_c54x_ld_smem_operands *operands)
{
    return validate_shape(instruction, expected_id, expected_resources,
                          expected_layout, expected_mode, operands);
}

ti_c54x_op_status ti_c54x_ld_smem_validate_reverse_trusted_operands(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    ti_c54x_opcode_id expected_id, ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_indirect_reverse_carry_mode expected_mode,
    ti_c54x_ld_smem_operands *operands)
{
    ti_c54x_ld_smem_operands decoded;
    const bool destination_present = has_destination(expected_layout);
    ti_c54x_op_status status;

    if (trusted_operands == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_shape(instruction, expected_id, expected_resources,
                            expected_layout, expected_mode, &decoded);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (trusted_operands->count < (destination_present ? UINT8_C(3) : UINT8_C(2)) ||
        (destination_present &&
         (trusted_operands->values[0].width_bits != UINT8_C(1) ||
          trusted_operands->values[0].raw_value > 1U)) ||
        trusted_operands->values[destination_present ? 1U : 0U].width_bits !=
            UINT8_C(3) ||
        trusted_operands->values[destination_present ? 1U : 0U].raw_value > 7U ||
        trusted_operands->values[destination_present ? 2U : 1U].width_bits !=
            UINT8_C(4) ||
        trusted_operands->values[destination_present ? 2U : 1U].raw_value !=
            expected_mode) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *operands = decoded;
    if (destination_present) {
        operands->destination =
            (uint8_t)trusted_operands->values[0].raw_value;
        operands->arf = (uint8_t)trusted_operands->values[1].raw_value;
    } else {
        operands->arf =
            (uint8_t)trusted_operands->values[0].raw_value;
    }
    if (operands->arf == UINT8_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

void ti_c54x_ld_smem_stage_reverse_memory_read(
    const ti_c54x_ld_smem_operands *operands,
    ti_c54x_indirect_reverse_carry_mode mode,
    ti_c54x_op_memory_complete_fn complete, ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {.kind = TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16,
                             .reverse_carry_mode = mode, .arf = operands->arf},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0), .complete = complete}};
}
