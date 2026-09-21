/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the two-word
 * FBD far branch-delayed carrying the 7-bit first-word page plus
 * the copied 16-bit extension verbatim into a DELAYED_FAR_PC_WRITE
 * effect with all
 * other state preserved.
 * Does not own: catalog lookup generation, program-counter or XPC
 * commit, sequencing, timing, buses, memory, diagnostics, or public
 * ABI declarations.
 */

#include "op_fbd.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status fbd_values(uint16_t pmad, uint8_t page,
                                        ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_DELAYED_FAR_PC_WRITE,
        .payload.delayed_far_pc_write = {.pmad = pmad, .page = page}};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status fbd_validate_instruction(
    const ti_c54x_decoded_instruction *instruction, uint16_t *pmad,
    uint8_t *page)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value page_field = {0U, 0U};
    ti_c54x_operand_field_value pmad_field = {0U, 0U};

    if (instruction == NULL || pmad == NULL || page == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_FBD);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_FBD ||
        metadata->first_word_value != UINT16_C(0xFA80) ||
        metadata->first_word_mask != UINT16_C(0xFF80) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_BRANCH_FAR_PMAD ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->id != TI_C54X_OPCODE_ID_FBD ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFF80)) != UINT16_C(0xFA80)) {
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
                &layout_view, &words, UINT8_C(0), &page_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            page_field.width_bits != UINT8_C(7) ||
            page_field.raw_value > UINT64_C(0x7F) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &pmad_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            pmad_field.width_bits != UINT8_C(16) ||
            pmad_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *page = (uint8_t)page_field.raw_value;
    *pmad = (uint16_t)pmad_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status fbd_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands, uint16_t *pmad,
    uint8_t *page)
{
    if (operands == NULL || pmad == NULL || page == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(7) ||
        operands->values[0].raw_value > UINT64_C(0x7F) ||
        operands->values[1].width_bits != UINT8_C(16) ||
        operands->values[1].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *page = (uint8_t)operands->values[0].raw_value;
    *pmad = (uint16_t)operands->values[1].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_fbd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint16_t pmad;
    uint8_t page;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = fbd_validate_instruction(instruction, &pmad, &page);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return fbd_values(pmad, page, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_fbd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint16_t pmad;
    uint8_t page;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status =
        fbd_validate_trusted_operands(operands, &pmad, &page);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return fbd_values(pmad, page, effect);
}
