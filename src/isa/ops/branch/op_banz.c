/*
 * Module contract
 *
 * Owns: pure checked/trusted BANZ pmad,Sind identity validation
 * and operand extraction, delegating Sind evaluation to the
 * shared branch-Sind helper with a clear delay flag.
 * Does not own: Sind evaluation, catalog lookup generation,
 * program-counter commit, AR writeback, countdown arming, CPU
 * sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations.
 */

#include "op_banz.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "op_branch_sind_common.h"

#define BANZ_FIRST_WORD_VALUE UINT16_C(0x6C00)
#define BANZ_FIRST_WORD_MASK UINT16_C(0xFF00)

static ti_c54x_op_status banz_extract(
    const ti_c54x_decoded_instruction *instruction, uint8_t *sind,
    uint16_t *pmad)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value sind_field = {0U, 0U};
    ti_c54x_operand_field_value pmad_field = {0U, 0U};

    if (instruction == NULL || sind == NULL || pmad == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_BANZ);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_BANZ ||
        metadata->first_word_value != BANZ_FIRST_WORD_VALUE ||
        metadata->first_word_mask != BANZ_FIRST_WORD_MASK ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_BRANCH_SIND_PMAD16 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS ||
        instruction->id != TI_C54X_OPCODE_ID_BANZ ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   BANZ_FIRST_WORD_MASK) != BANZ_FIRST_WORD_VALUE) {
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
                &layout_view, &words, UINT8_C(0), &sind_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            sind_field.width_bits != UINT8_C(8) ||
            sind_field.raw_value > UINT64_C(0xFF) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &pmad_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            pmad_field.width_bits != UINT8_C(16) ||
            pmad_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *sind = (uint8_t)sind_field.raw_value;
    *pmad = (uint16_t)pmad_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_banz(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t sind;
    uint16_t pmad;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = banz_extract(instruction, &sind, &pmad);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ti_c54x_branch_sind_evaluate(sind, pmad, input, false,
                                        effect);
}

ti_c54x_op_status ti_c54x_trusted_op_banz(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t sind;
    uint16_t pmad;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(8) ||
        operands->values[0].raw_value > UINT64_C(0xFF) ||
        operands->values[1].width_bits != UINT8_C(16) ||
        operands->values[1].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    sind = (uint8_t)operands->values[0].raw_value;
    pmad = (uint16_t)operands->values[1].raw_value;
    return ti_c54x_branch_sind_evaluate(sind, pmad, input, false,
                                        effect);
}
