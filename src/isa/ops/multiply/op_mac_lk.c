/* Module contract
 *
 * Owns: checked and trusted pure semantic leaves for MAC #lk,src[,dst]
 * syntax 3 (SPRU172C MAC[R] pp. 4-81-4-82): the UNROUNDED form only,
 * computing (T) times signed lk plus the S-selected source
 * accumulator into the D-selected destination accumulator through
 * the shared multiply core (FRCT shift, OVM saturation). The R
 * (rounded) suffix defers. T is READ, never written; C and TC are
 * preserved; OVdst goes sticky on 32-bit overflow (OVsrc when
 * dst==src). SMUL product pre-saturation stays out of scope on the
 * SMUL-0 baseline owned by the multiply core.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_mac_lk.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_multiply.h"

#define MAC_LK_FIRST_WORD_MASK UINT16_C(0xFCFF)
#define MAC_LK_FIRST_WORD_VALUE UINT16_C(0xF067)

typedef struct mac_lk_fields {
    uint8_t source;
    uint8_t destination;
    uint16_t immediate;
} mac_lk_fields;

static ti_c54x_op_status mac_lk_extract(
    const ti_c54x_decoded_instruction *instruction,
    mac_lk_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    uint64_t got[3] = {0U, 0U, 0U};
    uint8_t index;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16 ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   MAC_LK_FIRST_WORD_MASK) != MAC_LK_FIRST_WORD_VALUE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16,
            UINT8_C(2), &layout_view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < UINT8_C(3); ++index) {
        const uint8_t want_width =
            index == UINT8_C(2) ? UINT8_C(16) : UINT8_C(1);
        const uint64_t want_max =
            index == UINT8_C(2) ? UINT64_C(0xFFFF) : UINT64_C(1);
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != want_width ||
            field.raw_value > want_max) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        got[index] = field.raw_value;
    }
    fields->source = (uint8_t)got[0];
    fields->destination = (uint8_t)got[1];
    fields->immediate = (uint16_t)got[2];
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mac_lk_validate_inputs(
    const ti_c54x_op_input_snapshot *input, uint8_t source,
    uint8_t destination)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!input->t_register_known ||
        (source == UINT8_C(0) ? !input->accumulator_a_known
                             : !input->accumulator_b_known) ||
        (destination != source &&
         (destination == UINT8_C(0) ? !input->accumulator_a_known
                                   : !input->accumulator_b_known)) ||
        !input->status_frct_known || !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK ||
        input->accumulator_b_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (input->status_frct > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mac_lk_values(
    const mac_lk_fields *fields,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_multiply_operands operands;
    ti_c54x_multiply_result result;
    ti_c54x_multiply_status multiply_status;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_status status;

    if (fields == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mac_lk_validate_inputs(input, fields->source,
                                    fields->destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* The lk extension is the signed 16-bit multiplicand; T is read
     * but never written by this syntax. */
    operands.t_raw16 = input->t_register_raw16;
    operands.t_known = true;
    operands.multiplicand_raw16 = fields->immediate;
    operands.multiplicand_known = true;
    operands.accumulator_raw40 = fields->source == UINT8_C(0)
                                     ? input->accumulator_a_raw40
                                     : input->accumulator_b_raw40;
    operands.accumulator_known = true;
    operands.frct = input->status_frct;
    operands.frct_known = true;
    operands.ovm = input->status_ovm;
    operands.ovm_known = true;
    operands.rounding = false;
    operands.unsigned_operands = false;
    operands.unsigned_t_only = false;
    operands.select = TI_C54X_MULTIPLY_ADD;
    multiply_status = ti_c54x_multiply_accumulate(&operands, &result);
    if (multiply_status == TI_C54X_MULTIPLY_UNKNOWN) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (multiply_status != TI_C54X_MULTIPLY_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields->destination;
    staged.payload.accumulator_write.value_raw40 = result.accumulator_raw40;
    staged.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.destination_overflow_action =
        result.overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                        : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_mac_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    mac_lk_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mac_lk_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return mac_lk_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_mac_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    mac_lk_fields fields;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(3) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(1) ||
        operands->values[1].raw_value > UINT64_C(1) ||
        operands->values[2].width_bits != UINT8_C(16) ||
        operands->values[2].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields.source = (uint8_t)operands->values[0].raw_value;
    fields.destination = (uint8_t)operands->values[1].raw_value;
    fields.immediate = (uint16_t)operands->values[2].raw_value;
    return mac_lk_values(&fields, input, effect);
}
