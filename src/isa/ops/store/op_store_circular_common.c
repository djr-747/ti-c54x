/*
 * Module contract
 *
 * Owns: private checked/trusted validation and MEMORY-effect staging for
 * CMPT=0 circular MOD8/MOD10 WORD16 store wrappers. It validates copied
 * catalog/decode metadata and typed four-bit circular operands, admits AR0--AR7,
 * and retains no caller-owned pointer. Does not own: opcode-family payload
 * selection, opcode catalog generation, decode tables, address resolution,
 * buses, CPU state, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_store_circular_common.h"

#include <stddef.h>
#include <stdbool.h>

#include "isa/decode/operand_layout.h"

static bool circular_layout_supported(ti_c54x_operand_layout_id layout)
{
    return layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4 ||
           layout == TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4 ||
           layout == TI_C54X_OPERAND_LAYOUT_ARF_MOD4;
}

static bool circular_mode_supported(ti_c54x_indirect_circular_mode mode)
{
    return mode == TI_C54X_INDIRECT_CIRCULAR_MOD8 ||
           mode == TI_C54X_INDIRECT_CIRCULAR_MOD10;
}

static ti_c54x_op_status circular_validate_metadata(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_input_resource_mask expected_resources,
    const ti_c54x_opcode_metadata **metadata_out,
    ti_c54x_operand_layout_set_view *layout_view)
{
    const ti_c54x_opcode_metadata *metadata;

    if (instruction == NULL || metadata_out == NULL || layout_view == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (expected_id <= TI_C54X_OPCODE_ID_UNKNOWN ||
        expected_id >= TI_C54X_OPCODE_ID_COUNT ||
        !circular_layout_supported(expected_layout)) {
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
        instruction->bundle.program_word_count != metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    *metadata_out = metadata;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_store_validate_circular_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_store_circular_operands *operands)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field0 = {0U, 0U};
    ti_c54x_operand_field_value field1 = {0U, 0U};
    ti_c54x_operand_field_value field2 = {0U, 0U};
    ti_c54x_program_words_view words;
    ti_c54x_op_status status;

    if (instruction == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!circular_mode_supported(expected_mode)) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = circular_validate_metadata(
        instruction, expected_id, expected_layout, expected_resources,
        &metadata, &layout_view);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    if (expected_layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &field0) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &field1) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &field2) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field0.width_bits != UINT8_C(1) || field0.raw_value > UINT64_C(1) ||
            field1.width_bits != UINT8_C(3) || field1.raw_value > UINT64_C(7)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        operands->source = (uint8_t)field0.raw_value;
        operands->arf = (uint8_t)field1.raw_value;
    } else if (expected_layout ==
               TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &field0) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &field1) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &field2) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field0.width_bits != UINT8_C(3) || field0.raw_value > UINT64_C(7) ||
            field1.width_bits != UINT8_C(16) ||
            field1.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        operands->arf = (uint8_t)field0.raw_value;
        operands->extension = (uint16_t)field1.raw_value;
    } else {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &field0) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &field2) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field0.width_bits != UINT8_C(3) || field0.raw_value > UINT64_C(7)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        operands->arf = (uint8_t)field0.raw_value;
    }
    if (field2.width_bits != UINT8_C(4) ||
        field2.raw_value != (uint64_t)expected_mode) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    operands->mode = expected_mode;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_store_validate_circular_trusted_operands(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    ti_c54x_opcode_id expected_id,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_store_circular_operands *operands)
{
    ti_c54x_store_circular_operands decoded;
    ti_c54x_op_status status;
    uint8_t expected_count;

    if (trusted_operands == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_store_validate_circular_instruction(
        instruction, expected_id, expected_layout, expected_resources,
        expected_mode, &decoded);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    expected_count = expected_layout == TI_C54X_OPERAND_LAYOUT_ARF_MOD4
                         ? UINT8_C(2)
                         : UINT8_C(3);
    if (trusted_operands->count < expected_count) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (expected_layout == TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4) {
        if (trusted_operands->values[0].width_bits != UINT8_C(1) ||
            trusted_operands->values[0].raw_value > UINT64_C(1) ||
            trusted_operands->values[1].width_bits != UINT8_C(3) ||
            trusted_operands->values[1].raw_value > UINT64_C(7) ||
            trusted_operands->values[2].width_bits != UINT8_C(4) ||
            trusted_operands->values[2].raw_value !=
                (uint64_t)expected_mode) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        operands->source = (uint8_t)trusted_operands->values[0].raw_value;
        operands->arf = (uint8_t)trusted_operands->values[1].raw_value;
    } else if (expected_layout ==
               TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4) {
        if (trusted_operands->values[0].width_bits != UINT8_C(3) ||
            trusted_operands->values[0].raw_value > UINT64_C(7) ||
            trusted_operands->values[1].width_bits != UINT8_C(16) ||
            trusted_operands->values[1].raw_value > UINT64_C(0xFFFF) ||
            trusted_operands->values[2].width_bits != UINT8_C(4) ||
            trusted_operands->values[2].raw_value !=
                (uint64_t)expected_mode) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        operands->arf = (uint8_t)trusted_operands->values[0].raw_value;
        operands->extension = (uint16_t)trusted_operands->values[1].raw_value;
    } else {
        if (trusted_operands->values[0].width_bits != UINT8_C(3) ||
            trusted_operands->values[0].raw_value > UINT64_C(7) ||
            trusted_operands->values[1].width_bits != UINT8_C(4) ||
            trusted_operands->values[1].raw_value !=
                (uint64_t)expected_mode) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        operands->arf = (uint8_t)trusted_operands->values[0].raw_value;
    }
    operands->mode = expected_mode;
    return TI_C54X_OP_STATUS_OK;
}

void ti_c54x_store_stage_circular_write(
    uint8_t arf, ti_c54x_indirect_circular_mode mode, uint16_t value,
    ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16,
                .mode = TI_C54X_INDIRECT_SINGLE_MOD0,
                .indexed_mode = 0,
                .circular_mode = mode,
                .arf = arf},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .value_word = value,
            .complete = NULL}};
}
