/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for DST LONG32 MOD13.
 * Does not own: catalog/decode tables, address resolution or mutation, bus
 * execution, CPU state, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_dst_long32.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status dst_validate_shape(
    const ti_c54x_decoded_instruction *instruction)
{
    const ti_c54x_opcode_metadata *metadata;

    if (instruction == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_DST_LONG32_MOD13);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_DST_LONG32_MOD13 ||
        metadata->first_word_value != UINT16_C(0x4EE8) ||
        metadata->first_word_mask != UINT16_C(0xFEF8) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources !=
            (TI_C54X_INPUT_RESOURCES_ACCUMULATORS |
             TI_C54X_INPUT_RESOURCES_STATUS_SXM |
             TI_C54X_INPUT_RESOURCES_STATUS_SST) ||
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

static ti_c54x_op_status dst_extract_checked(
    const ti_c54x_decoded_instruction *instruction,
    uint8_t *source, uint8_t *arf, uint16_t *long_offset_word, uint8_t *mode)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value fields[4] = {{0U, 0U}};
    ti_c54x_program_words_view words;

    if (instruction == NULL || source == NULL || arf == NULL ||
        long_offset_word == NULL || mode == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    if (dst_validate_shape(instruction) != TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_DST_LONG32_MOD13);
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
    *source = (uint8_t)fields[0].raw_value;
    *arf = (uint8_t)fields[1].raw_value;
    *long_offset_word = (uint16_t)fields[2].raw_value;
    *mode = (uint8_t)fields[3].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status dst_validate_status(
    const ti_c54x_op_input_snapshot *input)
{
    if (!input->status_sxm_known || !input->status_sst_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_sxm > UINT8_C(1) || input->status_sst > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status dst_values(
    uint8_t source, uint8_t arf, uint16_t long_offset_word,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t raw;
    ti_c54x_u40_t stored;
    bool known;
    ti_c54x_op_effect staged;
    ti_c54x_op_status status;

    if (source > UINT8_C(1) || arf > UINT8_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = dst_validate_status(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (source == UINT8_C(0)) {
        raw = input->accumulator_a_raw40;
        known = input->accumulator_a_known;
    } else {
        raw = input->accumulator_b_raw40;
        known = input->accumulator_b_known;
    }
    if (!known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (raw > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    stored = raw;
    if (input->status_sst != UINT8_C(0)) {
        if (input->status_sxm == UINT8_C(0)) {
            if (raw > UINT64_C(0xFFFFFFFF)) {
                stored = UINT64_C(0xFFFFFFFF);
            }
        } else {
            const int64_t signed_raw = ti_c54x_u40_sign_extend(raw);
            if (signed_raw > INT64_C(0x7FFFFFFF)) {
                stored = UINT64_C(0x7FFFFFFF);
            } else if (signed_raw < -INT64_C(0x80000000)) {
                stored = UINT64_C(0x80000000);
            }
        }
    }
    stored &= UINT64_C(0xFFFFFFFF);
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32,
                .arf = arf,
                .long_offset_word = long_offset_word},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .first_value_msw = (uint16_t)(stored >> 16U),
            .second_value_lsw = (uint16_t)stored,
            .complete_long32 = NULL}};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_dst_long32_mod13(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint8_t source;
    uint8_t arf;
    uint8_t mode;
    uint16_t long_offset_word;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = dst_extract_checked(instruction, &source, &arf, &long_offset_word,
                                 &mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (mode != UINT8_C(13)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return dst_values(source, arf, long_offset_word, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_dst_long32_mod13(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    if (instruction == NULL || operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
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
    return dst_values((uint8_t)operands->values[0].raw_value,
                      (uint8_t)operands->values[1].raw_value,
                      (uint16_t)operands->values[2].raw_value, input, effect);
}
