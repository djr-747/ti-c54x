/* Module contract
 * Owns: pure checked/trusted SUB #lk semantics over the two shared
 * 2-word accumulator layouts: syn7 SUB #lk[,SHFT],src[,dst] widens
 * the 16-bit extension per SXM then shifts left by SHFT (0-15),
 * and syn8 SUB #lk,16,src[,dst] shifts the widened value left by
 * 16. Both compute src - (lk << shift) -> dst with OVM saturation
 * plus sticky OVdst. C is written from the low-32 no-borrow rule
 * (set iff no borrow); TC is preserved. SXM is consumed
 * (sign-extends a negative lk) and OVM is consumed; both must be
 * known.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, buses, CPU/state mutation, sequencing, timing,
 * observers, allocation, logging, environment, or diagnostics.
 */

#include "op_sub_lk.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "foundation/arithmetic40_addsub.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct sub_lk_fields {
    uint8_t source;
    uint8_t destination;
    uint8_t shift;
    uint16_t immediate;
} sub_lk_fields;

static ti_c54x_op_status sub_lk_extract(
    const ti_c54x_decoded_instruction *instruction,
    sub_lk_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    ti_c54x_operand_layout_id layout;
    uint16_t first_word;
    uint8_t operand_count;
    uint8_t want_width[4];
    uint64_t want_max[4];
    uint64_t got[4] = {0U, 0U, 0U, 0U};
    uint8_t index;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->bundle.program_word_count != UINT8_C(2)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    layout = instruction->operand_layout;
    first_word = instruction->bundle.program_words[0];
    if (layout ==
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16) {
        if ((uint16_t)(first_word & UINT16_C(0xFCF0)) != UINT16_C(0xF010)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        operand_count = UINT8_C(4);
        want_width[0] = UINT8_C(1);
        want_width[1] = UINT8_C(1);
        want_width[2] = UINT8_C(4);
        want_width[3] = UINT8_C(16);
        want_max[0] = UINT64_C(1);
        want_max[1] = UINT64_C(1);
        want_max[2] = UINT64_C(15);
        want_max[3] = UINT64_C(0xFFFF);
    } else if (layout ==
               TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16) {
        if ((uint16_t)(first_word & UINT16_C(0xFCFF)) != UINT16_C(0xF061)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        operand_count = UINT8_C(3);
        want_width[0] = UINT8_C(1);
        want_width[1] = UINT8_C(1);
        want_width[2] = UINT8_C(16);
        want_max[0] = UINT64_C(1);
        want_max[1] = UINT64_C(1);
        want_max[2] = UINT64_C(0xFFFF);
    } else {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(layout, UINT8_C(2),
                                           &layout_view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < operand_count; ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != want_width[index] ||
            field.raw_value > want_max[index]) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        got[index] = field.raw_value;
    }
    fields->source = (uint8_t)got[0];
    fields->destination = (uint8_t)got[1];
    if (operand_count == UINT8_C(4)) {
        fields->shift = (uint8_t)got[2];
        fields->immediate = (uint16_t)got[3];
    } else {
        fields->shift = UINT8_C(16);
        fields->immediate = (uint16_t)got[2];
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sub_lk_validate_inputs(
    const ti_c54x_op_input_snapshot *input, uint8_t source,
    uint8_t destination)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if ((source == UINT8_C(0) ? !input->accumulator_a_known
                             : !input->accumulator_b_known) ||
        (destination != source &&
         (destination == UINT8_C(0) ? !input->accumulator_a_known
                                   : !input->accumulator_b_known)) ||
        !input->status_sxm_known || !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK ||
        input->accumulator_b_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (input->status_sxm > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sub_lk_values(
    const sub_lk_fields *fields,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t subtrahend;
    ti_c54x_u40_t base;
    ti_c54x_u40_sub_result sub_result;
    ti_c54x_op_effect staged = {0};
    ti_c54x_u40_t value;
    bool no_borrow;
    ti_c54x_signed32_overflow_direction overflow;
    ti_c54x_op_status status;

    if (fields == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sub_lk_validate_inputs(input, fields->source,
                                    fields->destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* The lk extension is SXM-widened like the established LD leaves:
     * sign-extended when SXM=1 and bit 15 is set, else zero-extended.
     * The left shift then clears low bits. */
    subtrahend = ti_c54x_u40_from_unsigned(fields->immediate);
    if (input->status_sxm != UINT8_C(0) &&
        (fields->immediate & UINT16_C(0x8000)) != UINT16_C(0)) {
        subtrahend = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                     (uint64_t)fields->immediate;
    }
    subtrahend = ti_c54x_u40_logical_left_shift(subtrahend,
                                               (uint64_t)fields->shift);
    base = fields->source == UINT8_C(0) ? input->accumulator_a_raw40
                                       : input->accumulator_b_raw40;
    if (ti_c54x_u40_sub_checked(base, subtrahend, &sub_result) !=
        TI_C54X_ARITHMETIC40_ADDSUB_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    value = sub_result.value;
    no_borrow = sub_result.low32_no_borrow;
    overflow = sub_result.signed32_overflow;
    if (input->status_ovm != UINT8_C(0)) {
        if (overflow == TI_C54X_SIGNED32_OVERFLOW_POSITIVE) {
            value = UINT64_C(0x007fffffff);
        } else if (overflow == TI_C54X_SIGNED32_OVERFLOW_NEGATIVE) {
            value = UINT64_C(0xff80000000);
        }
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields->destination;
    staged.payload.accumulator_write.value_raw40 = value;
    staged.payload.accumulator_write.status_delta.c_action =
        no_borrow ? TI_C54X_OP_STATUS_BIT_WRITE_1
                  : TI_C54X_OP_STATUS_BIT_WRITE_0;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.destination_overflow_action =
        overflow == TI_C54X_SIGNED32_OVERFLOW_NONE
            ? TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE
            : TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_sub_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    sub_lk_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sub_lk_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sub_lk_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_op_sub_lk_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    if (instruction == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->operand_layout !=
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return ti_c54x_op_sub_lk(instruction, input, effect);
}

ti_c54x_op_status ti_c54x_op_sub_lk_16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    if (instruction == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->operand_layout !=
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return ti_c54x_op_sub_lk(instruction, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_sub_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    sub_lk_fields fields;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count == UINT8_C(4)) {
        if (operands->values[0].width_bits != UINT8_C(1) ||
            operands->values[0].raw_value > UINT64_C(1) ||
            operands->values[1].width_bits != UINT8_C(1) ||
            operands->values[1].raw_value > UINT64_C(1) ||
            operands->values[2].width_bits != UINT8_C(4) ||
            operands->values[2].raw_value > UINT64_C(15) ||
            operands->values[3].width_bits != UINT8_C(16) ||
            operands->values[3].raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        fields.source = (uint8_t)operands->values[0].raw_value;
        fields.destination = (uint8_t)operands->values[1].raw_value;
        fields.shift = (uint8_t)operands->values[2].raw_value;
        fields.immediate = (uint16_t)operands->values[3].raw_value;
    } else if (operands->count == UINT8_C(3)) {
        if (operands->values[0].width_bits != UINT8_C(1) ||
            operands->values[0].raw_value > UINT64_C(1) ||
            operands->values[1].width_bits != UINT8_C(1) ||
            operands->values[1].raw_value > UINT64_C(1) ||
            operands->values[2].width_bits != UINT8_C(16) ||
            operands->values[2].raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        fields.source = (uint8_t)operands->values[0].raw_value;
        fields.destination = (uint8_t)operands->values[1].raw_value;
        fields.shift = UINT8_C(16);
        fields.immediate = (uint16_t)operands->values[2].raw_value;
    } else {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return sub_lk_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_sub_lk_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    if (operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(4)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return ti_c54x_trusted_op_sub_lk(instruction, operands, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_sub_lk_16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    if (operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return ti_c54x_trusted_op_sub_lk(instruction, operands, input, effect);
}
