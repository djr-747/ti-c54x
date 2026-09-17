/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for one-word
 * XC n,cond with taken NONE emission versus not-taken
 * CONDITIONAL_SKIP staging. The sixteen image-evidenced single
 * conditions evaluate through the shared branch-condition helper
 * and UNC takes unconditionally; BIO/NBIO, overflow, and
 * multi-condition combination fields have no input carrier and
 * reject as deferred INVALID_OPERAND. Each leaf validates
 * identity and operands and stages no status, accumulator,
 * T-register, or MEMORY effect.
 * Does not own: catalog lookup generation, program-counter commit,
 * skip application, delay-slot accounting, sequencing, timing,
 * buses, memory, diagnostics, or public ABI declarations.
 */

#include "isa/ops/branch/op_xc.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/branch/op_branch_cond_common.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#define XC_COND_UNC UINT8_C(0x00)

static ti_c54x_op_status xc_validate_instruction(
    const ti_c54x_decoded_instruction *instruction)
{
    const ti_c54x_opcode_metadata *metadata;

    if (instruction == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_XC);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_XC ||
        metadata->first_word_value != UINT16_C(0xFD00) ||
        metadata->first_word_mask != UINT16_C(0xFD00) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_N_CONDITION8 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_XC ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_N_CONDITION8 ||
        instruction->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        instruction->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC ||
        instruction->bundle.program_word_count != UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if ((uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFD00)) != UINT16_C(0xFD00)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status xc_evaluate(uint8_t cond,
                                     const ti_c54x_op_input_snapshot *input,
                                     bool *taken)
{
    if (input == NULL || taken == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (cond == XC_COND_UNC) {
        *taken = true;
        return TI_C54X_OP_STATUS_OK;
    }
    return ti_c54x_branch_cond_evaluate(cond, input, taken);
}

static ti_c54x_op_status xc_emit(bool taken, uint8_t skip_words,
                                 ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (skip_words != UINT8_C(1) && skip_words != UINT8_C(2)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (taken) {
        staged = (ti_c54x_op_effect){.kind = TI_C54X_OP_EFFECT_NONE};
    } else {
        staged = (ti_c54x_op_effect){
            .kind = TI_C54X_OP_EFFECT_CONDITIONAL_SKIP,
            .payload.conditional_skip = {.skip_words = skip_words}};
    }
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status xc_extract(
    const ti_c54x_decoded_instruction *instruction, uint8_t *select,
    uint8_t *cond)
{
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value n_field = {0U, 0U};
    ti_c54x_operand_field_value cond_field = {0U, 0U};
    ti_c54x_program_words_view words;

    if (instruction == NULL || select == NULL || cond == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_N_CONDITION8, UINT8_C(1), &view) !=
            TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
        view.descriptor == NULL ||
        view.descriptor->operand_count != UINT8_C(2)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words = (ti_c54x_program_words_view){
        instruction->bundle.program_words,
        instruction->bundle.program_word_count};
    if (ti_c54x_operand_layout_set_extract_operand(&view, &words,
                                                   UINT8_C(0),
                                                   &n_field) !=
            TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
        n_field.width_bits != UINT8_C(1) || n_field.raw_value > UINT64_C(1) ||
        ti_c54x_operand_layout_set_extract_operand(&view, &words,
                                                   UINT8_C(1),
                                                   &cond_field) !=
            TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
        cond_field.width_bits != UINT8_C(8) ||
        cond_field.raw_value > UINT64_C(0xFF)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    *select = (uint8_t)n_field.raw_value;
    *cond = (uint8_t)cond_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_xc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t select;
    uint8_t cond;
    bool taken;
    ti_c54x_op_status status;

    status = xc_validate_instruction(instruction);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = xc_extract(instruction, &select, &cond);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = xc_evaluate(cond, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return xc_emit(taken, (uint8_t)(select + UINT8_C(1)), effect);
}

ti_c54x_op_status ti_c54x_trusted_op_xc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint64_t select;
    uint64_t cond;
    bool taken;
    ti_c54x_op_status status;

    status = xc_validate_instruction(instruction);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(2)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    select = operands->values[0].raw_value;
    cond = operands->values[1].raw_value;
    if (operands->values[0].width_bits != UINT8_C(1) || select > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(8) ||
        cond > UINT64_C(0xFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if ((uint8_t)select !=
            (uint8_t)((instruction->bundle.program_words[0] >> 9U) & UINT16_C(1)) ||
        (uint8_t)cond !=
            (uint8_t)(instruction->bundle.program_words[0] & UINT16_C(0xFF))) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = xc_evaluate((uint8_t)cond, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return xc_emit(taken, (uint8_t)((uint8_t)select + UINT8_C(1)), effect);
}
