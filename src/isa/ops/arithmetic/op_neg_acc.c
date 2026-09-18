/* Module contract
 * Owns: pure checked/trusted NEG src[,dst] semantics (dst40 =
 * 40-bit two's complement of src40) over the shared
 * ACCUMULATOR_SOURCE_DESTINATION layout. OVM saturates a
 * signed-32 overflowing result (positive edge to 0x00_7FFF_FFFF,
 * negative edge to 0xFF_8000_0000) with sticky OVdst; C is
 * written (1 iff src == 0) and TC is preserved.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, accumulator/status mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_neg_acc.h"

#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "foundation/arithmetic40_addsub.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define NEG_ACC_FIXED_MASK UINT16_C(0xFCFF)
#define NEG_ACC_FIXED_VALUE UINT16_C(0xF484)
#define NEG_ACC_POSITIVE_SATURATE UINT64_C(0x007FFFFFFF)
#define NEG_ACC_NEGATIVE_SATURATE UINT64_C(0xFF80000000)

typedef struct neg_acc_fields {
    uint8_t source;
    uint8_t destination;
} neg_acc_fields;

static ti_c54x_op_status neg_acc_extract(
    const ti_c54x_decoded_instruction *instruction,
    neg_acc_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    uint8_t index;
    uint8_t got[2] = {0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_NEG_ACC ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   NEG_ACC_FIXED_MASK) != NEG_ACC_FIXED_VALUE) {
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

static ti_c54x_op_status neg_acc_validate_inputs(
    const ti_c54x_op_input_snapshot *input, uint8_t source)
{
    ti_c54x_u40_t source_raw40;

    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    source_raw40 = source == UINT8_C(0) ? input->accumulator_a_raw40
                                        : input->accumulator_b_raw40;
    if ((source == UINT8_C(0) ? !input->accumulator_a_known
                              : !input->accumulator_b_known) ||
        !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (source_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static bool neg_acc_overflows_signed32(ti_c54x_u40_t value)
{
    uint64_t high = value >> UINT64_C(31);

    return high != UINT64_C(0) && high != UINT64_C(0x1FF);
}

static ti_c54x_op_status neg_acc_compute(
    const neg_acc_fields *fields,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40;
    ti_c54x_u40_sub_result sub_result;
    ti_c54x_u40_t value;
    bool overflow;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_status status;

    if (fields == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = neg_acc_validate_inputs(input, fields->source);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    source_raw40 = fields->source == UINT8_C(0)
                       ? input->accumulator_a_raw40
                       : input->accumulator_b_raw40;
    if (ti_c54x_u40_sub_checked(UINT64_C(0), source_raw40,
                                &sub_result) !=
        TI_C54X_ARITHMETIC40_ADDSUB_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    value = sub_result.value;
    /* The wrapped negation overflows signed-32 whenever bits 39:31
     * are not all equal; that covers both the 0x80_0000_0000 wrap
     * and the 0xFF_8000_0000 to 0x00_8000_0000 edge from the TI NEG
     * prose. Saturation follows the true result sign, which is the
     * inverse of the source sign. */
    overflow = neg_acc_overflows_signed32(value);
    if (overflow && input->status_ovm != UINT8_C(0)) {
        value = (source_raw40 & TI_C54X_U40_SIGN_BIT) != UINT64_C(0)
                    ? NEG_ACC_POSITIVE_SATURATE
                    : NEG_ACC_NEGATIVE_SATURATE;
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields->destination;
    staged.payload.accumulator_write.value_raw40 = value;
    staged.payload.accumulator_write.status_delta.c_action =
        source_raw40 == UINT64_C(0) ? TI_C54X_OP_STATUS_BIT_WRITE_1
                                    : TI_C54X_OP_STATUS_BIT_WRITE_0;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.destination_overflow_action =
        overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                 : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_neg_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    neg_acc_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = neg_acc_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return neg_acc_compute(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_neg_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    neg_acc_fields fields;

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
    return neg_acc_compute(&fields, input, effect);
}
