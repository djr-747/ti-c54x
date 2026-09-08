/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for DLD LONG32 MOD13.
 * Does not own: catalog/decode tables, address resolution or mutation, bus
 * execution, CPU state, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_dld_long32.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status dld_validate_shape(
    const ti_c54x_decoded_instruction *instruction)
{
    const ti_c54x_opcode_metadata *metadata;

    if (instruction == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_DLD_LONG32_MOD13);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_DLD_LONG32_MOD13 ||
        metadata->first_word_value != UINT16_C(0x56E8) ||
        metadata->first_word_mask != UINT16_C(0xFEF8) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_STATUS_SXM ||
        instruction->id != metadata->id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status dld_extract_checked(
    const ti_c54x_decoded_instruction *instruction,
    uint8_t *destination, uint8_t *arf, uint16_t *long_offset_word,
    uint8_t *mode)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value fields[4] = {{0U, 0U}};
    ti_c54x_program_words_view words;

    if (instruction == NULL || destination == NULL || arf == NULL ||
        long_offset_word == NULL || mode == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    if (dld_validate_shape(instruction) != TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_DLD_LONG32_MOD13);
    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    for (uint8_t index = 0U; index < UINT8_C(4); ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &fields[index]) !=
            TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    if (fields[0].width_bits != UINT8_C(1) || fields[0].raw_value > 1U ||
        fields[1].width_bits != UINT8_C(3) || fields[1].raw_value > 7U ||
        fields[2].width_bits != UINT8_C(16) || fields[2].raw_value > 0xffffU ||
        fields[3].width_bits != UINT8_C(4) || fields[3].raw_value != 13U) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    *destination = (uint8_t)fields[0].raw_value;
    *arf = (uint8_t)fields[1].raw_value;
    *long_offset_word = (uint16_t)fields[2].raw_value;
    *mode = (uint8_t)fields[3].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status dld_validate_sxm(
    const ti_c54x_op_input_snapshot *input)
{
    if (!input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    return input->status_sxm <= UINT8_C(1)
               ? TI_C54X_OP_STATUS_OK
               : TI_C54X_OP_STATUS_INVALID_OPERAND;
}

static ti_c54x_op_status dld_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t first_msw,
    uint16_t second_lsw, ti_c54x_op_effect *next_effect)
{
    ti_c54x_op_effect staged;
    ti_c54x_u40_t value;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = dld_validate_shape(instruction);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = dld_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    value = ti_c54x_u40_from_unsigned(
        ((uint64_t)first_msw << 16U) | (uint64_t)second_lsw);
    if (input->status_sxm != UINT8_C(0) &&
        (first_msw & UINT16_C(0x8000)) != UINT16_C(0)) {
        value = ti_c54x_u40_from_unsigned(value | UINT64_C(0xFF00000000));
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = (uint8_t)((instruction->bundle.program_words[0] >> 8U) &
                                     UINT16_C(1)),
            .value_raw40 = value,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE}}};
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static void dld_stage_read(uint8_t arf, uint16_t long_offset_word,
                           ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};
    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32;
    staged.payload.memory.address_spec.arf = arf;
    staged.payload.memory.address_spec.long_offset_word = long_offset_word;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.complete_long32 = dld_complete;
    *effect = staged;
}

ti_c54x_op_status ti_c54x_op_dld_long32_mod13(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint8_t destination;
    uint8_t arf;
    uint8_t mode;
    uint16_t long_offset_word;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = dld_extract_checked(instruction, &destination, &arf,
                                 &long_offset_word, &mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)destination;
    if (mode != UINT8_C(13)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    status = dld_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    dld_stage_read(arf, long_offset_word, effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_dld_long32_mod13(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    if (instruction == NULL || operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    (void)instruction;
    if (operands->count != UINT8_C(4) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > 1U ||
        operands->values[1].width_bits != UINT8_C(3) ||
        operands->values[1].raw_value > 7U ||
        operands->values[2].width_bits != UINT8_C(16) ||
        operands->values[2].raw_value > 0xffffU ||
        operands->values[3].width_bits != UINT8_C(4) ||
        operands->values[3].raw_value != 13U) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    {
        const ti_c54x_op_status status = dld_validate_sxm(input);
        if (status != TI_C54X_OP_STATUS_OK) {
            return status;
        }
    }
    dld_stage_read((uint8_t)operands->values[1].raw_value,
                   (uint16_t)operands->values[2].raw_value, effect);
    return TI_C54X_OP_STATUS_OK;
}
