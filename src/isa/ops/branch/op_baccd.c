/*
 * Module contract
 *
 * Owns: pure checked/trusted BACCD semantics carrying the selected
 * accumulator low word into a PC_WRITE effect with all other state
 * preserved.
 * Does not own: catalog lookup generation, program-counter commit,
 * CPU sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations.
 */

#include "op_baccd.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status baccd_values(uint16_t target,
                                     ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_DELAYED_PC_WRITE,
        .payload.delayed_pc_write = {.pmad = target}};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status baccd_target_from_snapshot(
    uint8_t select, const ti_c54x_op_input_snapshot *input,
    uint16_t *target)
{
    ti_c54x_u40_t raw40 = UINT64_C(0);
    bool known = false;

    if (input == NULL || target == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (select == UINT8_C(0)) {
        raw40 = input->accumulator_a_raw40;
        known = input->accumulator_a_known;
    } else if (select == UINT8_C(1)) {
        raw40 = input->accumulator_b_raw40;
        known = input->accumulator_b_known;
    } else {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (!known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if ((raw40 & (ti_c54x_u40_t)~TI_C54X_U40_MASK) != UINT64_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *target = (uint16_t)(raw40 & UINT64_C(0xFFFF));
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status baccd_validate_instruction(
    const ti_c54x_decoded_instruction *instruction, uint8_t *select)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value select_field = {0U, 0U};

    if (instruction == NULL || select == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_BACCD);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_BACCD ||
        metadata->first_word_value != UINT16_C(0xF6E2) ||
        metadata->first_word_mask != UINT16_C(0xFEFF) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_ACCUMULATORS ||
        instruction->id != TI_C54X_OPCODE_ID_BACCD ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFEFF)) != UINT16_C(0xF6E2)) {
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
                &layout_view, &words, UINT8_C(0), &select_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            select_field.width_bits != UINT8_C(1) ||
            select_field.raw_value > UINT64_C(1)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *select = (uint8_t)select_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status baccd_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands, uint8_t *select)
{
    if (operands == NULL || select == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *select = (uint8_t)operands->values[0].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_baccd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t select;
    uint16_t target;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = baccd_validate_instruction(instruction, &select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = baccd_target_from_snapshot(select, input, &target);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return baccd_values(target, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_baccd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t select;
    uint16_t target;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = baccd_validate_trusted_operands(operands, &select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = baccd_target_from_snapshot(select, input, &target);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return baccd_values(target, effect);
}
