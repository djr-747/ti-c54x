/*
 * Module contract
 *
 * Owns: pure checked/trusted BCD pmad,cond semantics over the sixteen
 * evidenced single conditions with taken DELAYED_PC_WRITE and
 * not-taken NONE emission and consumed-input-only knownness.
 * Single-condition evaluation is shared with the BC leaf through
 * the branch-condition helper; this leaf owns only identity
 * validation and emission.
 * Does not own: catalog lookup generation, program-counter commit,
 * countdown arming, CPU sequencing, timing, buses, memory,
 * diagnostics, or public ABI declarations.
 */

#include "op_bcd.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "op_branch_cond_common.h"

static ti_c54x_op_status bcd_emit(bool taken, uint16_t pmad,
                                  ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (taken) {
        staged = (ti_c54x_op_effect){
            .kind = TI_C54X_OP_EFFECT_DELAYED_PC_WRITE,
            .payload.delayed_pc_write = {.pmad = pmad}};
    } else {
        staged = (ti_c54x_op_effect){.kind = TI_C54X_OP_EFFECT_NONE};
    }
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status bcd_extract(
    const ti_c54x_decoded_instruction *instruction, uint8_t *cond,
    uint16_t *pmad)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value cond_field = {0U, 0U};
    ti_c54x_operand_field_value pmad_field = {0U, 0U};

    if (instruction == NULL || cond == NULL || pmad == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_BCD);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_BCD ||
        metadata->first_word_value != UINT16_C(0xFA00) ||
        metadata->first_word_mask != UINT16_C(0xFF80) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC ||
        instruction->id != TI_C54X_OPCODE_ID_BCD ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFF80)) != UINT16_C(0xFA00)) {
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
                &layout_view, &words, UINT8_C(0), &cond_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            cond_field.width_bits != UINT8_C(8) ||
            cond_field.raw_value > UINT64_C(0xFF) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &pmad_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            pmad_field.width_bits != UINT8_C(16) ||
            pmad_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *cond = (uint8_t)cond_field.raw_value;
    *pmad = (uint16_t)pmad_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_bcd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t cond;
    uint16_t pmad;
    bool taken;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = bcd_extract(instruction, &cond, &pmad);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_branch_cond_evaluate(cond, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return bcd_emit(taken, pmad, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_bcd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t cond;
    uint16_t pmad;
    bool taken;
    ti_c54x_op_status status;

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
    cond = (uint8_t)operands->values[0].raw_value;
    pmad = (uint16_t)operands->values[1].raw_value;
    status = ti_c54x_branch_cond_evaluate(cond, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return bcd_emit(taken, pmad, effect);
}
