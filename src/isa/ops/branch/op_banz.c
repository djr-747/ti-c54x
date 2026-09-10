/*
 * Module contract
 *
 * Owns: pure checked/trusted BANZ pmad,Sind semantics over the
 * image-evidenced I=1/MOD0 (*ARx) and I=1/MOD1 (*ARx-)
 * combinations with always-applied side effects (post-decrement
 * value-after on MOD1, observation on MOD0, even when not taken)
 * and taken/not-taken BRANCH_AUXILIARY emission with a clear
 * delayed flag, gated on known selected AR. All other Sind
 * combinations reject as deferred before any input is consumed.
 * Does not own: catalog lookup generation, program-counter commit,
 * AR writeback, countdown arming, CPU sequencing, timing, buses,
 * memory, diagnostics, or public ABI declarations.
 */

#include "op_banz.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define BANZ_FIRST_WORD_VALUE UINT16_C(0x6C00)
#define BANZ_FIRST_WORD_MASK UINT16_C(0xFF00)
#define BANZ_SIND_INDIRECT_BIT UINT8_C(0x80)
#define BANZ_SIND_MOD_SHIFT UINT8_C(3)
#define BANZ_SIND_MOD_MASK UINT8_C(0x0F)
#define BANZ_SIND_MOD_OBSERVE UINT8_C(0)
#define BANZ_SIND_MOD_POSTDECREMENT UINT8_C(1)
#define BANZ_SIND_ARX_MASK UINT8_C(0x07)

static ti_c54x_op_status banz_parse_sind(uint8_t sind, uint8_t *ar_index,
                                         bool *decrement)
{
    uint8_t mod;

    if (ar_index == NULL || decrement == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if ((uint8_t)(sind & BANZ_SIND_INDIRECT_BIT) == UINT8_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    mod = (uint8_t)((uint8_t)(sind >> BANZ_SIND_MOD_SHIFT) &
                    BANZ_SIND_MOD_MASK);
    if (mod != BANZ_SIND_MOD_OBSERVE &&
        mod != BANZ_SIND_MOD_POSTDECREMENT) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *ar_index = (uint8_t)(sind & BANZ_SIND_ARX_MASK);
    *decrement = mod == BANZ_SIND_MOD_POSTDECREMENT;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status banz_emit(
    const ti_c54x_op_input_snapshot *input, uint8_t ar_index,
    bool decrement, uint16_t pmad, ti_c54x_op_effect *effect)
{
    uint16_t value;
    ti_c54x_op_effect staged;

    if (input == NULL || effect == NULL || ar_index >= UINT8_C(8)) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (((uint8_t)(input->auxiliary_registers_known_mask >>
                   ar_index) &
         UINT8_C(1)) == UINT8_C(0)) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    value = input->auxiliary_registers_raw16[ar_index];
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_BRANCH_AUXILIARY,
        .payload.branch_auxiliary = {
            .ar_index = ar_index,
            .value_after = (uint16_t)(decrement ? value - UINT16_C(1)
                                                : value),
            .update = decrement,
            .taken = value != UINT16_C(0),
            .delayed = false,
            .pmad = pmad}};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

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
    uint8_t ar_index;
    bool decrement;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = banz_extract(instruction, &sind, &pmad);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = banz_parse_sind(sind, &ar_index, &decrement);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return banz_emit(input, ar_index, decrement, pmad, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_banz(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t sind;
    uint16_t pmad;
    uint8_t ar_index;
    bool decrement;
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
    sind = (uint8_t)operands->values[0].raw_value;
    pmad = (uint16_t)operands->values[1].raw_value;
    status = banz_parse_sind(sind, &ar_index, &decrement);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return banz_emit(input, ar_index, decrement, pmad, effect);
}
