/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the exact
 * two-word RPTB pmad block repeat (TI SPRU172C section
 * RPTB[D] with Z=0: first word 0xF072, 16-bit block-end
 * program address, execution sets BRAF and loads RSA
 * with PC+2 and REA with pmad; the block runs BRC+1
 * times, BRC preloaded). The checked leaf requires the
 * RPTB opcode id, validates the BRANCH_PMAD16 layout id
 * and 2-word count, and extracts pmad through the layout
 * set; the trusted leaf takes 1 operand with exact width
 * (16). Both paths share one values core that stages a
 * fire-and-forget REPEAT_BLOCK_START effect carrying the
 * copied pmad. Only the evidenced Z=0 first word
 * composes; the Z=1 delayed form rejects. Borrowed
 * instruction, operand, input, and effect objects are used
 * for one call only; no pointer is retained and failed
 * calls leave the caller effect unchanged.
 * Does not own: opcode inventory, catalog rows, dispatch bindings, operand
 * layouts, BRC/RSA/REA state, address resolution, buses, CPU/state
 * mutation, sequencing, timing, observers, diagnostics, allocation,
 * logging, or environment.
 */

#include "op_rptb.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status rptb_values(uint16_t pmad,
                                     ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_REPEAT_BLOCK_START,
        .payload.repeat_block_start = {.pmad = pmad}};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status rptb_validate_instruction(
    const ti_c54x_decoded_instruction *instruction, uint16_t *pmad)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value pmad_field = {0U, 0U};

    if (instruction == NULL || pmad == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RPTB);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_RPTB ||
        metadata->first_word_value != UINT16_C(0xF072) ||
        metadata->first_word_mask != UINT16_C(0xFFFF) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->id != TI_C54X_OPCODE_ID_RPTB ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        instruction->bundle.program_words[0] != UINT16_C(0xF072)) {
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
                &layout_view, &words, UINT8_C(0), &pmad_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            pmad_field.width_bits != UINT8_C(16) ||
            pmad_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *pmad = (uint16_t)pmad_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status rptb_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands, uint16_t *pmad)
{
    if (operands == NULL || pmad == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(16) ||
        operands->values[0].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *pmad = (uint16_t)operands->values[0].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_rptb(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint16_t pmad;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    (void)input;
    status = rptb_validate_instruction(instruction, &pmad);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return rptb_values(pmad, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_rptb(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint16_t pmad;
    ti_c54x_op_status status;

    if (instruction == NULL || operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    (void)instruction;
    (void)input;
    status = rptb_validate_trusted_operands(operands, &pmad);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return rptb_values(pmad, effect);
}
