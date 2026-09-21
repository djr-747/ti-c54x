/* Module contract
 * Owns: pure checked/trusted SUB src,ASM semantics (syntax 10:
 * dst40 = dst40 minus the accumulator source shifted by the signed
 * 5-bit ASM count) over the shared ACCUMULATOR_SOURCE_DESTINATION
 * layout. Left shifts are logical; right shifts are arithmetic under
 * SXM=1 and logical under SXM=0. OVM saturates the 32-bit-signed
 * overflowing result with sticky OVdst; C is written from the
 * bit-32 borrow (C set means no borrow) and TC is preserved.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, accumulator/status mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_sub_src_asm.h"

#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "foundation/arithmetic40_addsub.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define SUB_SRC_ASM_FIXED_MASK UINT16_C(0xFCFF)
#define SUB_SRC_ASM_FIXED_VALUE UINT16_C(0xF481)

typedef struct sub_src_asm_fields {
    uint8_t source;
    uint8_t destination;
} sub_src_asm_fields;

static ti_c54x_u40_t sub_src_asm_shift(
    ti_c54x_u40_t source_raw40, int32_t shift, uint8_t sxm)
{
    if (shift >= INT32_C(0)) {
        return ti_c54x_u40_logical_left_shift(
            source_raw40, (uint64_t)shift);
    }
    if (sxm != UINT8_C(0)) {
        return ti_c54x_u40_arithmetic_right_shift(
            source_raw40, (uint64_t)(-shift));
    }
    return ti_c54x_u40_mask(source_raw40 >> (uint64_t)(-shift));
}

static ti_c54x_op_status sub_src_asm_extract(
    const ti_c54x_decoded_instruction *instruction,
    sub_src_asm_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    uint8_t index;
    uint8_t got[2] = {0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   SUB_SRC_ASM_FIXED_MASK) != SUB_SRC_ASM_FIXED_VALUE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION,
            UINT8_C(1),
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < UINT8_C(2); ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != UINT8_C(1) ||
            field.raw_value > UINT64_C(1)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        got[index] = (uint8_t)field.raw_value;
    }
    fields->source = got[0];
    fields->destination = got[1];
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sub_src_asm_validate_inputs(
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
        !input->status_sxm_known || !input->status_ovm_known ||
        !input->status_asm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK ||
        input->accumulator_b_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (input->status_sxm > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1) ||
        input->status_asm > UINT8_C(31)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sub_src_asm_compute(
    const sub_src_asm_fields *fields,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40;
    ti_c54x_u40_t base_raw40;
    int32_t shift;
    ti_c54x_u40_t subtrahend;
    ti_c54x_u40_sub_result sub_result;
    ti_c54x_u40_t value;
    bool no_borrow;
    ti_c54x_signed32_overflow_direction overflow;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_status status;

    if (fields == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sub_src_asm_validate_inputs(input, fields->source,
                                         fields->destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    source_raw40 = fields->source == UINT8_C(0)
                       ? input->accumulator_a_raw40
                       : input->accumulator_b_raw40;
    base_raw40 = fields->destination == UINT8_C(0)
                     ? input->accumulator_a_raw40
                     : input->accumulator_b_raw40;
    shift = input->status_asm <= UINT8_C(15)
                ? (int32_t)input->status_asm
                : (int32_t)input->status_asm - INT32_C(32);
    subtrahend = sub_src_asm_shift(source_raw40, shift,
                                   input->status_sxm);
    if (ti_c54x_u40_sub_checked(base_raw40, subtrahend, &sub_result) !=
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

ti_c54x_op_status ti_c54x_op_sub_src_asm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    sub_src_asm_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sub_src_asm_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sub_src_asm_compute(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_sub_src_asm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    sub_src_asm_fields fields;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(2)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields.source = (uint8_t)operands->values[0].raw_value;
    fields.destination = (uint8_t)operands->values[1].raw_value;
    if (operands->values[0].width_bits != UINT8_C(1) ||
        fields.source > UINT8_C(1) ||
        operands->values[1].width_bits != UINT8_C(1) ||
        fields.destination > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return sub_src_asm_compute(&fields, input, effect);
}
