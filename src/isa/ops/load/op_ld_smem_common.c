/*
 * Module contract
 *
 * Owns: private shared validation and MEMORY-effect staging for the neutral
 * standard-indirect accumulator loads. It extracts the copied destination,
 * ARF, and MOD operands from a catalog/decode contract and stages copied
 * single-indirect, indexed, or typed circular WORD16 reads. It retains no
 * caller-owned pointer and leaves caller effects unchanged on failure.
 * Does not own: opcode-family completion transforms, SXM/T input semantics,
 * decode tables, address resolution, buses, CPU state, sequencing, timing,
 * observers, allocation, logging, environment, or diagnostics.
 */

#include "op_ld_smem_common.h"

#include <stdbool.h>
#include <stddef.h>

#include "isa/decode/operand_layout.h"

ti_c54x_op_status ti_c54x_ld_smem_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_ld_smem_operands *operands)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value arf_field = {0U, 0U};
    ti_c54x_operand_field_value mode_field = {0U, 0U};

    if (instruction == NULL || operands == NULL) {
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
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD ||
        metadata->repeatability < TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != expected_resources ||
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
                &layout_view, &words, UINT8_C(0), &destination_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &arf_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &mode_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            destination_field.width_bits != UINT8_C(1) ||
            destination_field.raw_value > UINT64_C(1) ||
            arf_field.width_bits != UINT8_C(3) ||
            arf_field.raw_value > UINT64_C(7) ||
            mode_field.width_bits != UINT8_C(2) ||
            mode_field.raw_value > UINT64_C(3)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    operands->destination = (uint8_t)destination_field.raw_value;
    operands->arf = (uint8_t)arf_field.raw_value;
    operands->mode = (uint8_t)mode_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_ld_smem_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands,
    ti_c54x_ld_smem_operands *decoded_operands)
{
    if (operands == NULL || decoded_operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(3) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(3) ||
        operands->values[1].raw_value > UINT64_C(7) ||
        operands->values[2].width_bits != UINT8_C(2) ||
        operands->values[2].raw_value > UINT64_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    decoded_operands->destination = (uint8_t)operands->values[0].raw_value;
    decoded_operands->arf = (uint8_t)operands->values[1].raw_value;
    decoded_operands->mode = (uint8_t)operands->values[2].raw_value;
    decoded_operands->shift_raw5 = UINT8_C(0);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_ld_smem_validate_shift_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_ld_smem_operands *operands)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value arf_field = {0U, 0U};
    ti_c54x_operand_field_value mode_field = {0U, 0U};
    ti_c54x_operand_field_value shift_field = {0U, 0U};

    if (instruction == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value & (uint16_t)~metadata->first_word_mask) !=
            UINT16_C(0) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD_SHIFT5 ||
        metadata->repeatability < TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != expected_resources ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        instruction->bundle.program_word_count != metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value ||
        (uint16_t)(instruction->bundle.program_words[1] &
                   UINT16_C(0xFEE0)) != UINT16_C(0x0C40)) {
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
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &arf_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &mode_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(3), &shift_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            destination_field.width_bits != UINT8_C(1) ||
            destination_field.raw_value > UINT64_C(1) ||
            arf_field.width_bits != UINT8_C(3) ||
            arf_field.raw_value > UINT64_C(7) ||
            mode_field.width_bits != UINT8_C(2) ||
            mode_field.raw_value > UINT64_C(3) ||
            shift_field.width_bits != UINT8_C(5) ||
            shift_field.raw_value > UINT64_C(0x1F)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    operands->destination = (uint8_t)destination_field.raw_value;
    operands->arf = (uint8_t)arf_field.raw_value;
    operands->mode = (uint8_t)mode_field.raw_value;
    operands->shift_raw5 = (uint8_t)shift_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_ld_smem_validate_shift_trusted_operands(
    const ti_c54x_trusted_operand_values *operands,
    ti_c54x_ld_smem_operands *decoded_operands)
{
    if (operands == NULL || decoded_operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(4) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(3) ||
        operands->values[1].raw_value > UINT64_C(7) ||
        operands->values[2].width_bits != UINT8_C(2) ||
        operands->values[2].raw_value > UINT64_C(3) ||
        operands->values[3].width_bits != UINT8_C(5) ||
        operands->values[3].raw_value > UINT64_C(0x1F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    decoded_operands->destination = (uint8_t)operands->values[0].raw_value;
    decoded_operands->arf = (uint8_t)operands->values[1].raw_value;
    decoded_operands->mode = (uint8_t)operands->values[2].raw_value;
    decoded_operands->shift_raw5 = (uint8_t)operands->values[3].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_ld_smem_validate_mode(uint8_t mode)
{
    return mode >= (uint8_t)TI_C54X_INDIRECT_SINGLE_MOD3
               ? TI_C54X_OP_STATUS_INVALID_OPERAND
               : TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_ld_smem_validate_indexed_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    uint8_t expected_mode,
    ti_c54x_ld_smem_operands *operands)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value arf_field = {0U, 0U};
    ti_c54x_operand_field_value mode_field = {0U, 0U};
    bool has_destination;

    if (instruction == NULL || operands == NULL ||
        (expected_mode != (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5 &&
         expected_mode != (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6)) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    has_destination =
        expected_layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4;
    if (!has_destination && expected_layout != TI_C54X_OPERAND_LAYOUT_ARF_MOD4) {
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
        metadata->operand_layout != expected_layout ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != expected_resources ||
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
        const ti_c54x_operand_layout_status arf_status =
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, has_destination ? UINT8_C(1) : UINT8_C(0),
                &arf_field);
        const ti_c54x_operand_layout_status mode_status =
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, has_destination ? UINT8_C(2) : UINT8_C(1),
                &mode_field);

        if ((has_destination &&
             ti_c54x_operand_layout_set_extract_operand(
                 &layout_view, &words, UINT8_C(0), &destination_field) !=
                 TI_C54X_OPERAND_LAYOUT_STATUS_OK) ||
            arf_status != TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            mode_status != TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            (has_destination &&
             (destination_field.width_bits != UINT8_C(1) ||
              destination_field.raw_value > UINT64_C(1))) ||
            arf_field.width_bits != UINT8_C(3) ||
            arf_field.raw_value > UINT64_C(7) ||
            mode_field.width_bits != UINT8_C(4) ||
            mode_field.raw_value != expected_mode) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    operands->destination = has_destination
                                ? (uint8_t)destination_field.raw_value
                                : UINT8_C(0);
    operands->arf = (uint8_t)arf_field.raw_value;
    operands->mode = (uint8_t)mode_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_ld_smem_validate_indexed_trusted_operands(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    uint8_t expected_mode,
    ti_c54x_ld_smem_operands *operands)
{
    ti_c54x_ld_smem_operands decoded;
    const bool has_destination =
        expected_layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4;
    ti_c54x_op_status status;

    if (trusted_operands == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_indexed_instruction(
        instruction, expected_id, expected_resources, expected_layout,
        expected_mode, &decoded);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (trusted_operands->count < (has_destination ? UINT8_C(3) : UINT8_C(2)) ||
        (has_destination &&
         (trusted_operands->values[0].width_bits != UINT8_C(1) ||
          trusted_operands->values[0].raw_value > UINT64_C(1))) ||
        trusted_operands->values[has_destination ? 1U : 0U].width_bits !=
            UINT8_C(3) ||
        trusted_operands->values[has_destination ? 1U : 0U].raw_value >
            UINT64_C(7) ||
        trusted_operands->values[has_destination ? 2U : 1U].width_bits !=
            UINT8_C(4) ||
        trusted_operands->values[has_destination ? 2U : 1U].raw_value !=
            expected_mode) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    operands->destination = has_destination
                                ? (uint8_t)trusted_operands->values[0].raw_value
                                : UINT8_C(0);
    operands->arf = (uint8_t)trusted_operands->values[has_destination ? 1U : 0U]
                        .raw_value;
    operands->mode = expected_mode;
    return TI_C54X_OP_STATUS_OK;
}

void ti_c54x_ld_smem_stage_memory_read(
    const ti_c54x_ld_smem_operands *operands,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16,
                .mode = (ti_c54x_indirect_single_mode)operands->mode,
                .arf = operands->arf},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0),
            .complete = complete}};
    *effect = staged;
}

void ti_c54x_ld_smem_stage_indexed_memory_read(
    const ti_c54x_ld_smem_operands *operands,
    uint8_t indexed_mode,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16,
                .mode = TI_C54X_INDIRECT_SINGLE_MOD0,
                .indexed_mode = (ti_c54x_indirect_indexed_mode)indexed_mode,
                .arf = operands->arf},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0),
            .complete = complete}};
    *effect = staged;
}

static ti_c54x_op_status validate_circular_metadata(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_ld_smem_operands *operands)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value arf_field = {0U, 0U};
    ti_c54x_operand_field_value mode_field = {0U, 0U};
    const bool has_destination =
        expected_layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4;
    const bool valid_layout =
        has_destination || expected_layout == TI_C54X_OPERAND_LAYOUT_ARF_MOD4;

    if (instruction == NULL || operands == NULL || !valid_layout ||
        (expected_mode != TI_C54X_INDIRECT_CIRCULAR_MOD8 &&
         expected_mode != TI_C54X_INDIRECT_CIRCULAR_MOD10)) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value & (uint16_t)~metadata->first_word_mask) !=
            UINT16_C(0) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout != expected_layout ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != expected_resources ||
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
        const uint8_t arf_index = has_destination ? UINT8_C(1) : UINT8_C(0);
        const uint8_t mode_index = has_destination ? UINT8_C(2) : UINT8_C(1);
        if ((has_destination &&
             ti_c54x_operand_layout_set_extract_operand(
                 &layout_view, &words, UINT8_C(0), &destination_field) !=
                 TI_C54X_OPERAND_LAYOUT_STATUS_OK) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, arf_index, &arf_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, mode_index, &mode_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            (has_destination &&
             (destination_field.width_bits != UINT8_C(1) ||
              destination_field.raw_value > UINT64_C(1))) ||
            arf_field.width_bits != UINT8_C(3) ||
            arf_field.raw_value > UINT64_C(7) ||
            mode_field.width_bits != UINT8_C(4) ||
            mode_field.raw_value != (uint64_t)expected_mode) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    operands->destination = has_destination
                                ? (uint8_t)destination_field.raw_value
                                : UINT8_C(0);
    operands->arf = (uint8_t)arf_field.raw_value;
    operands->mode = (uint8_t)expected_mode;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_ld_smem_validate_circular_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_ld_smem_operands *operands)
{
    return validate_circular_metadata(instruction, expected_id,
                                      expected_resources, expected_layout,
                                      expected_mode, operands);
}

ti_c54x_op_status ti_c54x_ld_smem_validate_circular_trusted_operands(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_ld_smem_operands *operands)
{
    ti_c54x_ld_smem_operands decoded;
    const bool has_destination =
        expected_layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4;
    ti_c54x_op_status status;

    if (trusted_operands == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_circular_metadata(instruction, expected_id,
                                        expected_resources, expected_layout,
                                        expected_mode, &decoded);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (trusted_operands->count < (has_destination ? UINT8_C(3) : UINT8_C(2)) ||
        (has_destination &&
         (trusted_operands->values[0].width_bits != UINT8_C(1) ||
          trusted_operands->values[0].raw_value > UINT64_C(1))) ||
        trusted_operands->values[has_destination ? 1U : 0U].width_bits !=
            UINT8_C(3) ||
        trusted_operands->values[has_destination ? 1U : 0U].raw_value >
            UINT64_C(7) ||
        trusted_operands->values[has_destination ? 2U : 1U].width_bits !=
            UINT8_C(4) ||
        trusted_operands->values[has_destination ? 2U : 1U].raw_value !=
            (uint64_t)expected_mode) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    operands->destination = has_destination
                                ? (uint8_t)trusted_operands->values[0].raw_value
                                : UINT8_C(0);
    operands->arf = (uint8_t)trusted_operands
                        ->values[has_destination ? 1U : 0U]
                        .raw_value;
    operands->mode = (uint8_t)expected_mode;
    return TI_C54X_OP_STATUS_OK;
}

void ti_c54x_ld_smem_stage_circular_memory_read(
    const ti_c54x_ld_smem_operands *operands,
    ti_c54x_indirect_circular_mode mode,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16,
                .circular_mode = mode,
                .arf = operands->arf},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0),
            .complete = complete}};
}
