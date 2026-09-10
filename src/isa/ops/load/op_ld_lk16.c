/*
 * Module contract
 *
 * Owns: pure checked/trusted LD #lk,16,dst semantics over a copied
 * 16-bit extension SXM-widened and shifted left 16 into the
 * destination accumulator with all status bits preserved.
 * Does not own: catalog lookup generation, accumulator/status mutation,
 * CPU sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations.
 */

#include "op_ld_lk16.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct ld_lk16_operands {
    uint8_t destination;
    uint16_t immediate;
} ld_lk16_operands;

static ti_c54x_op_status ld_lk16_validate_sxm(
    const ti_c54x_op_input_snapshot *input)
{
    if (!input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_sxm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_lk16_values(uint8_t destination,
                                        uint16_t immediate,
                                        const ti_c54x_op_input_snapshot *input,
                                        ti_c54x_op_effect *effect)
{
    ti_c54x_op_status input_status = ld_lk16_validate_sxm(input);
    ti_c54x_u40_t value_raw40;
    ti_c54x_op_effect staged;

    if (input_status != TI_C54X_OP_STATUS_OK) {
        return input_status;
    }

    value_raw40 = ti_c54x_u40_from_unsigned(immediate);
    if (input->status_sxm != UINT8_C(0) &&
        (immediate & UINT16_C(0x8000)) != UINT16_C(0)) {
        value_raw40 = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                      (uint64_t)immediate;
    }
    value_raw40 = ti_c54x_u40_logical_left_shift(value_raw40, UINT8_C(16));
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = value_raw40,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .destination_overflow_action =
                    TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE}}};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_lk16_validate_instruction(
    const ti_c54x_decoded_instruction *instruction, ld_lk16_operands *operands)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value immediate_field = {0U, 0U};

    if (instruction == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_LK_16);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_LD_LK_16 ||
        metadata->first_word_value != UINT16_C(0xF062) ||
        metadata->first_word_mask != UINT16_C(0xFEFF) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_STATUS_SXM ||
        instruction->id != TI_C54X_OPCODE_ID_LD_LK_16 ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFEFF)) != UINT16_C(0xF062)) {
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
                &layout_view, &words, UINT8_C(1), &immediate_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            destination_field.width_bits != UINT8_C(1) ||
            destination_field.raw_value > UINT64_C(1) ||
            immediate_field.width_bits != UINT8_C(16) ||
            immediate_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    operands->destination = (uint8_t)destination_field.raw_value;
    operands->immediate = (uint16_t)immediate_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_lk16_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands,
    ld_lk16_operands *decoded_operands)
{
    if (operands == NULL || decoded_operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(16) ||
        operands->values[1].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    decoded_operands->destination = (uint8_t)operands->values[0].raw_value;
    decoded_operands->immediate = (uint16_t)operands->values[1].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_ld_lk16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ld_lk16_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_lk16_validate_instruction(instruction, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ld_lk16_values(operands.destination, operands.immediate, input,
                          effect);
}

ti_c54x_op_status ti_c54x_trusted_op_ld_lk16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ld_lk16_operands decoded_operands;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_lk16_validate_trusted_operands(operands, &decoded_operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ld_lk16_values(decoded_operands.destination,
                          decoded_operands.immediate, input, effect);
}
