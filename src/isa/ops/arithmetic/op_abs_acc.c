/* Module contract
 * Owns: pure checked/trusted ABS src[,dst] semantics (dst40 =
 * absolute value of src40 with 40-bit wrap) over the shared
 * ACCUMULATOR_SOURCE_DESTINATION layout. OVM saturates a
 * guard-overflowing magnitude to 007FFFFFFFh with sticky OVdst;
 * C is set from a zero stored result and TC is preserved.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, accumulator/status mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_abs_acc.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "foundation/arithmetic40_addsub.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define ABS_ACC_FIXED_MASK UINT16_C(0xFCFF)
#define ABS_ACC_FIXED_VALUE UINT16_C(0xF485)
#define ABS_ACC_MIN_NEGATIVE UINT64_C(0x8000000000)
#define ABS_ACC_GUARD_MASK UINT64_C(0xFF00000000)
#define ABS_ACC_POSITIVE_SATURATE UINT64_C(0x007FFFFFFF)

typedef struct abs_acc_fields {
    uint8_t source;
    uint8_t destination;
} abs_acc_fields;

static ti_c54x_op_status abs_acc_extract(
    const ti_c54x_decoded_instruction *instruction,
    abs_acc_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    uint8_t index;
    uint8_t got[2] = {0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_ABS_ACC ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   ABS_ACC_FIXED_MASK) != ABS_ACC_FIXED_VALUE) {
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

static ti_c54x_op_status abs_acc_validate_inputs(
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

static ti_c54x_op_status abs_acc_compute(
    const abs_acc_fields *fields,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40;
    ti_c54x_u40_t magnitude;
    ti_c54x_u40_t value;
    bool overflow;
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_status status;

    if (fields == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = abs_acc_validate_inputs(input, fields->source);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    source_raw40 = fields->source == UINT8_C(0)
                       ? input->accumulator_a_raw40
                       : input->accumulator_b_raw40;
    if (source_raw40 == ABS_ACC_MIN_NEGATIVE) {
        magnitude = ABS_ACC_MIN_NEGATIVE;
    } else if ((source_raw40 & TI_C54X_U40_SIGN_BIT) != UINT64_C(0)) {
        magnitude = ti_c54x_u40_mask(UINT64_C(0x10000000000) -
                                     source_raw40);
    } else {
        magnitude = source_raw40;
    }
    overflow = (magnitude & ABS_ACC_GUARD_MASK) != UINT64_C(0);
    value = magnitude;
    if (input->status_ovm != UINT8_C(0) && overflow) {
        value = ABS_ACC_POSITIVE_SATURATE;
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields->destination;
    staged.payload.accumulator_write.value_raw40 = value;
    staged.payload.accumulator_write.status_delta.c_action =
        value == UINT64_C(0) ? TI_C54X_OP_STATUS_BIT_WRITE_1
                             : TI_C54X_OP_STATUS_BIT_WRITE_0;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.destination_overflow_action =
        overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                 : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_abs_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    abs_acc_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = abs_acc_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return abs_acc_compute(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_abs_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    abs_acc_fields fields;

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
    return abs_acc_compute(&fields, input, effect);
}
