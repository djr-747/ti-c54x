/* Module contract
 * Owns: pure checked/trusted CMPR CC,ARx unsigned comparison and construction
 * of one TC status-bit effect.  Every successful call stages the effect from
 * copied input; rejected calls leave caller effect storage unchanged.
 * Does not own: catalog lookup, architectural state, input collection, status
 * mutation, CPU sequencing, timing, memory, I/O, allocation, or diagnostics.
 */

#include "op_cmpr.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../../catalog/opcode_catalog.h"
#include "../../decode/operand_layout.h"
#include "../dispatch/trusted_semantics.h"

static bool metadata_is_valid(const ti_c54x_opcode_metadata *metadata)
{
    return metadata != NULL && metadata->id == TI_C54X_OPCODE_ID_CMPR_AR &&
           metadata->first_word_mask != UINT16_C(0) &&
           (metadata->first_word_value &
            (uint16_t)~metadata->first_word_mask) == UINT16_C(0) &&
           metadata->program_word_count == UINT8_C(1) &&
           metadata->operand_layout == TI_C54X_OPERAND_LAYOUT_CONDITION_ARF &&
           metadata->repeatability == TI_C54X_REPEATABILITY_INELIGIBLE &&
           metadata->input_resources ==
               TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS;
}

static bool condition_holds(uint8_t cc, uint16_t selected, uint16_t ar0)
{
    switch (cc) {
    case 0U:
        return selected == ar0;
    case 1U:
        return selected < ar0;
    case 2U:
        return selected > ar0;
    default:
        return selected != ar0;
    }
}

static ti_c54x_op_status compare_values(uint8_t cc, uint8_t arx,
                                        const ti_c54x_op_input_snapshot *input,
                                        ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;
    bool condition;

    if (cc > UINT8_C(3) || arx > UINT8_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if ((input->auxiliary_registers_known_mask & UINT8_C(1)) == UINT8_C(0) ||
        (input->auxiliary_registers_known_mask &
         (uint8_t)(UINT8_C(1) << arx)) == UINT8_C(0)) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    condition = condition_holds(
        cc, input->auxiliary_registers_raw16[arx],
        input->auxiliary_registers_raw16[UINT8_C(0)]);
    staged = (ti_c54x_op_effect){0};
    staged.kind = TI_C54X_OP_EFFECT_STATUS_BIT_WRITE;
    staged.payload.status_bit_write.target_register =
        TI_C54X_OP_STATUS_REGISTER_ST0;
    staged.payload.status_bit_write.bit_index = UINT8_C(12);
    staged.payload.status_bit_write.raw_value = condition ? UINT8_C(1) : UINT8_C(0);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status checked_cmpr(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value cc_field = {0U, 0U};
    ti_c54x_operand_field_value arx_field = {0U, 0U};

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_CMPR_AR);
    if (!metadata_is_valid(metadata) || instruction->id != metadata->id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        instruction->bundle.program_word_count != metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
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
                &layout_view, &words, UINT8_C(0), &cc_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &arx_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            cc_field.width_bits != UINT8_C(2) ||
            arx_field.width_bits != UINT8_C(3) || cc_field.raw_value > 3U ||
            arx_field.raw_value > 7U) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    return compare_values((uint8_t)cc_field.raw_value,
                          (uint8_t)arx_field.raw_value, input, effect);
}

ti_c54x_op_status ti_c54x_op_cmpr(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return checked_cmpr(instruction, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_cmpr(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL ||
        operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(2) ||
        operands->values[1].width_bits != UINT8_C(3) ||
        operands->values[0].raw_value > UINT64_C(3) ||
        operands->values[1].raw_value > UINT64_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    return compare_values((uint8_t)operands->values[0].raw_value,
                          (uint8_t)operands->values[1].raw_value, input,
                          effect);
}
