/* Module contract
 * Owns: checked and trusted pure two-word STH-by-SHIFT WORD16
 * memory-write effects over one signed-SHIFT-shifted accumulator high
 * word (bits 31-16 of the accumulator shifted arithmetically by the
 * signed 5-bit second-word SHIFT count, -16 to +15), one direct-DP,
 * MOD0-MOD3 standard-indirect, or MOD4-family indirect address, and one
 * source select. SXM is provably
 * not an input: every encoded shift is a pure shift-and-extract over
 * source bits, so SXM rides hostile and unconsumed. The two-word form
 * carries no ARP alias, so ARF0 is a plain selector here, unlike the
 * single-word reverse-carry leaves.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "op_sth_shift_smem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_store_indexed_common.h"
#include "op_store_circular_common.h"
#include "op_store_reverse_carry_common.h"

/* Second-word fixed pattern: bits 15:9 carry 0x06, bits 7:5 are fixed
 * 011. Bit 8 is the source select and bits 4:0 are the signed SHIFT, so
 * both stay outside the mask (0xFEE0). */
#define STH_SHIFT_WORD2_MASK UINT16_C(0xFEE0)
#define STH_SHIFT_WORD2_VALUE UINT16_C(0x0C60)

/* DP is a 9-bit ST0 field. */
#define STH_SHIFT_DP_MAX UINT16_C(0x01FF)

/* Shift the accumulator arithmetically by the signed SHIFT count
 * (non-negative counts shift left, negative counts shift right by the
 * negation) and extract bits 31-16. */
static uint16_t sth_shift_shifted_high(uint64_t source_raw40, int8_t shift)
{
    ti_c54x_u40_t shifted;

    if (shift >= INT8_C(0)) {
        shifted = ti_c54x_u40_logical_left_shift(source_raw40,
                                                 (uint64_t)shift);
    } else {
        shifted = ti_c54x_u40_arithmetic_right_shift(
            source_raw40, (uint64_t)(-(int64_t)shift));
    }
    return (uint16_t)((shifted >> 16U) & UINT64_C(0xFFFF));
}

/* Interpret a raw 5-bit field as a signed SHIFT count in -16 to +15. */
static int8_t sth_shift_sign_extend5(uint64_t raw5)
{
    uint8_t field = (uint8_t)(raw5 & UINT64_C(0x1F));

    return (field >= UINT8_C(16)) ? (int8_t)(field - UINT8_C(32))
                                  : (int8_t)field;
}

static ti_c54x_op_status sth_shift_check_word2(uint16_t word1)
{
    if ((uint16_t)(word1 & STH_SHIFT_WORD2_MASK) != STH_SHIFT_WORD2_VALUE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sth_shift_check_metadata(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_input_resource_mask expected_resources,
    uint8_t expected_word_count)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(expected_id);

    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count != expected_word_count ||
        metadata->operand_layout != expected_layout ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != expected_resources ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return TI_C54X_OP_STATUS_OK;
}

typedef struct sth_shift_direct_operands {
    uint8_t source;
    uint8_t address;
    int8_t shift;
} sth_shift_direct_operands;

typedef struct sth_shift_indirect_operands {
    uint8_t source;
    uint8_t arf;
    uint8_t mode;
    int8_t shift;
} sth_shift_indirect_operands;

static ti_c54x_op_status sth_shift_extract_direct(
    const ti_c54x_decoded_instruction *instruction,
    sth_shift_direct_operands *operands)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value address_field = {0U, 0U};
    ti_c54x_operand_field_value shift_field = {0U, 0U};
    ti_c54x_op_status status;

    status = sth_shift_check_metadata(
        instruction, TI_C54X_OPCODE_ID_STH_SHIFT_DIRECT,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DIRECT7_SHIFT5,
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_DP, UINT8_C(2));
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DIRECT7_SHIFT5,
            UINT8_C(2), &layout_view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &source_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &address_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &shift_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            source_field.width_bits != UINT8_C(1) ||
            address_field.width_bits != UINT8_C(7) ||
            shift_field.width_bits != UINT8_C(5) ||
            source_field.raw_value > UINT64_C(1) ||
            address_field.raw_value > UINT64_C(0x7F) ||
            shift_field.raw_value > UINT64_C(0x1F)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    status = sth_shift_check_word2(instruction->bundle.program_words[1]);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    operands->source = (uint8_t)source_field.raw_value;
    operands->address = (uint8_t)address_field.raw_value;
    operands->shift = sth_shift_sign_extend5(shift_field.raw_value);
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sth_shift_extract_indirect(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id, uint8_t expected_mode,
    sth_shift_indirect_operands *operands)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value arf_field = {0U, 0U};
    ti_c54x_operand_field_value mode_field = {0U, 0U};
    ti_c54x_operand_field_value shift_field = {0U, 0U};
    ti_c54x_op_status status;

    status = sth_shift_check_metadata(
        instruction, expected_id,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5,
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS,
        UINT8_C(2));
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5,
            UINT8_C(2), &layout_view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &source_field) !=
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
            source_field.width_bits != UINT8_C(1) ||
            arf_field.width_bits != UINT8_C(3) ||
            mode_field.width_bits != UINT8_C(4) ||
            shift_field.width_bits != UINT8_C(5) ||
            source_field.raw_value > UINT64_C(1) ||
            arf_field.raw_value > UINT64_C(7) ||
            mode_field.raw_value != (uint64_t)expected_mode ||
            shift_field.raw_value > UINT64_C(0x1F)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    status = sth_shift_check_word2(instruction->bundle.program_words[1]);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    operands->source = (uint8_t)source_field.raw_value;
    operands->arf = (uint8_t)arf_field.raw_value;
    operands->mode = expected_mode;
    operands->shift = sth_shift_sign_extend5(shift_field.raw_value);
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sth_shift_select_source(
    uint8_t source, const ti_c54x_op_input_snapshot *input,
    uint64_t *source_raw40)
{
    bool source_known;

    if (source > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (source == UINT8_C(0)) {
        *source_raw40 = input->accumulator_a_raw40;
        source_known = input->accumulator_a_known;
    } else {
        *source_raw40 = input->accumulator_b_raw40;
        source_known = input->accumulator_b_known;
    }
    if (!source_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (*source_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sth_shift_direct_values(
    const sth_shift_direct_operands *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint64_t source_raw40 = UINT64_C(0);
    ti_c54x_op_status status;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->source > UINT8_C(1) ||
        operands->address > UINT8_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = sth_shift_select_source(operands->source, input, &source_raw40);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (!input->status_dp_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_dp > STH_SHIFT_DP_MAX) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                /* DIRECT_DP reinterprets the shared long/offset fields:
                 * DP page in `long_offset_word`, offset in
                 * `direct_dma7` (no arm growth; ABI pinned). */
                .kind = TI_C54X_OP_MEMORY_ADDRESS_DIRECT_DP_WORD16,
                .long_offset_word = input->status_dp,
                .direct_dma7 = operands->address},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .value_word =
                sth_shift_shifted_high(source_raw40, operands->shift),
            .complete = NULL}};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sth_shift_indirect_values(
    const sth_shift_indirect_operands *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint64_t source_raw40 = UINT64_C(0);
    uint16_t value;
    ti_c54x_op_status status;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->source > UINT8_C(1) || operands->arf > UINT8_C(7) ||
        (operands->mode != UINT8_C(0) && operands->mode != UINT8_C(1) &&
         operands->mode != UINT8_C(2) && operands->mode != UINT8_C(3) &&
         operands->mode != UINT8_C(4) && operands->mode != UINT8_C(5) &&
         operands->mode != UINT8_C(6) && operands->mode != UINT8_C(7) &&
         operands->mode != UINT8_C(8) && operands->mode != UINT8_C(10))) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = sth_shift_select_source(operands->source, input, &source_raw40);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    value = sth_shift_shifted_high(source_raw40, operands->shift);
    switch (operands->mode) {
    case UINT8_C(0):
    case UINT8_C(1):
    case UINT8_C(2):
    case UINT8_C(3):
        *effect = (ti_c54x_op_effect){
            .kind = TI_C54X_OP_EFFECT_MEMORY,
            .payload.memory = {
                .address_spec = {
                    .kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16,
                    .mode = (ti_c54x_indirect_single_mode)operands->mode,
                    .arf = operands->arf},
                .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                .value_word = value,
                .complete = NULL}};
        return TI_C54X_OP_STATUS_OK;
    case UINT8_C(5):
        ti_c54x_store_stage_indexed_write(
            operands->arf, (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, value,
            effect);
        break;
    case UINT8_C(6):
        ti_c54x_store_stage_indexed_write(
            operands->arf, (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, value,
            effect);
        break;
    case UINT8_C(8):
        ti_c54x_store_stage_circular_write(
            operands->arf, TI_C54X_INDIRECT_CIRCULAR_MOD8, value, effect);
        break;
    case UINT8_C(10):
        ti_c54x_store_stage_circular_write(
            operands->arf, TI_C54X_INDIRECT_CIRCULAR_MOD10, value, effect);
        break;
    case UINT8_C(4):
        ti_c54x_store_stage_reverse_carry_write(
            operands->arf, TI_C54X_INDIRECT_REVERSE_CARRY_MOD4, value,
            effect);
        break;
    default:
        ti_c54x_store_stage_reverse_carry_write(
            operands->arf, TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, value,
            effect);
        break;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_sth_shift_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    sth_shift_direct_operands operands = {0U, 0U, INT8_C(0)};
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sth_shift_extract_direct(instruction, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sth_shift_direct_values(&operands, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_sth_shift_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    sth_shift_direct_operands decoded = {0U, 0U, INT8_C(0)};
    sth_shift_direct_operands trusted;
    ti_c54x_op_status status;

    if (instruction == NULL || operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sth_shift_extract_direct(instruction, &decoded);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (operands->count < UINT8_C(3) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(7) ||
        operands->values[1].raw_value > UINT64_C(0x7F) ||
        operands->values[2].width_bits != UINT8_C(5)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    /* The trusted SHIFT arrives as an 8-bit two's-complement count; only
     * the low five bits are significant. */
    trusted.source = (uint8_t)operands->values[0].raw_value;
    trusted.address = (uint8_t)operands->values[1].raw_value;
    trusted.shift =
        sth_shift_sign_extend5(operands->values[2].raw_value);
    return sth_shift_direct_values(&trusted, input, effect);
}

static ti_c54x_op_status sth_shift_indirect_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id, uint8_t expected_mode)
{
    sth_shift_indirect_operands operands = {0U, 0U, 0U, INT8_C(0)};
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status =
        sth_shift_extract_indirect(instruction, expected_id, expected_mode,
                                   &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sth_shift_indirect_values(&operands, input, effect);
}

static ti_c54x_op_status sth_shift_indirect_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id, uint8_t expected_mode)
{
    sth_shift_indirect_operands decoded = {0U, 0U, 0U, INT8_C(0)};
    sth_shift_indirect_operands trusted;
    ti_c54x_op_status status;

    if (instruction == NULL || trusted_operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sth_shift_extract_indirect(instruction, expected_id,
                                        expected_mode, &decoded);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (trusted_operands->count < UINT8_C(4) ||
        trusted_operands->values[0].width_bits != UINT8_C(1) ||
        trusted_operands->values[0].raw_value > UINT64_C(1) ||
        trusted_operands->values[1].width_bits != UINT8_C(3) ||
        trusted_operands->values[1].raw_value > UINT64_C(7) ||
        trusted_operands->values[2].width_bits != UINT8_C(4) ||
        trusted_operands->values[2].raw_value !=
            (uint64_t)expected_mode ||
        trusted_operands->values[3].width_bits != UINT8_C(5)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    trusted.source = (uint8_t)trusted_operands->values[0].raw_value;
    trusted.arf = (uint8_t)trusted_operands->values[1].raw_value;
    trusted.mode = expected_mode;
    trusted.shift =
        sth_shift_sign_extend5(trusted_operands->values[3].raw_value);
    return sth_shift_indirect_values(&trusted, input, effect);
}

ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_checked(instruction, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD0,
                                      UINT8_C(0));
}

ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_checked(instruction, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD1,
                                      UINT8_C(1));
}

ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_checked(instruction, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD2,
                                      UINT8_C(2));
}

ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_checked(instruction, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD3,
                                      UINT8_C(3));
}

ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod4(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_checked(instruction, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD4,
                                      UINT8_C(4));
}

ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_checked(instruction, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD5,
                                      UINT8_C(5));
}

ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_checked(instruction, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD6,
                                      UINT8_C(6));
}

ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod7(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_checked(instruction, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD7,
                                      UINT8_C(7));
}

ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod8(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_checked(instruction, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD8,
                                      UINT8_C(8));
}

ti_c54x_op_status ti_c54x_op_sth_shift_smem_mod10(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_checked(instruction, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD10,
                                      UINT8_C(10));
}

ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_trusted(instruction, operands, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD0,
                                      UINT8_C(0));
}

ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_trusted(instruction, operands, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD1,
                                      UINT8_C(1));
}

ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_trusted(instruction, operands, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD2,
                                      UINT8_C(2));
}

ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_trusted(instruction, operands, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD3,
                                      UINT8_C(3));
}

ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod4(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_trusted(instruction, operands, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD4,
                                      UINT8_C(4));
}

ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_trusted(instruction, operands, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD5,
                                      UINT8_C(5));
}

ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_trusted(instruction, operands, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD6,
                                      UINT8_C(6));
}

ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod7(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_trusted(instruction, operands, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD7,
                                      UINT8_C(7));
}

ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod8(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_trusted(instruction, operands, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD8,
                                      UINT8_C(8));
}

ti_c54x_op_status ti_c54x_trusted_op_sth_shift_smem_mod10(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return sth_shift_indirect_trusted(instruction, operands, input, effect,
                                      TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD10,
                                      UINT8_C(10));
}
