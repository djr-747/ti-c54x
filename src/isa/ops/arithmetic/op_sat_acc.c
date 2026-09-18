/* Module contract
 * Owns: pure checked/trusted SAT src semantics (src40 clamped
 * to a signed 32-bit value written back to src) over the shared
 * ACCUMULATOR_SOURCE layout. Saturation applies regardless of
 * OVM; OVsrc is recomputed (set when the value clamps, cleared
 * otherwise per the TI SAT examples), and C and TC are
 * preserved.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, accumulator/status mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_sat_acc.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define SAT_ACC_FIXED_MASK UINT16_C(0xFEFF)
#define SAT_ACC_FIXED_VALUE UINT16_C(0xF483)
#define SAT_ACC_POSITIVE_LIMIT UINT64_C(0x007fffffff)
#define SAT_ACC_NEGATIVE_LIMIT UINT64_C(0xff80000000)

static ti_c54x_op_status sat_acc_extract(
    const ti_c54x_decoded_instruction *instruction, uint8_t *source)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;

    if (instruction == NULL || source == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_SAT_ACC ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   SAT_ACC_FIXED_MASK) != SAT_ACC_FIXED_VALUE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE, UINT8_C(1),
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    if (ti_c54x_operand_layout_set_extract_operand(
            &layout_view, &words, UINT8_C(0), &field) !=
            TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
        field.width_bits != UINT8_C(1) ||
        field.raw_value > UINT64_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    *source = (uint8_t)field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static void sat_acc_saturate(
    ti_c54x_u40_t source_raw40, ti_c54x_u40_t *value, bool *clamped)
{
    uint64_t guard;
    bool low_negative;

    guard = source_raw40 >> UINT64_C(32);
    low_negative =
        (source_raw40 & UINT64_C(0x80000000)) != UINT64_C(0);
    if ((guard == UINT64_C(0) && !low_negative) ||
        (guard == UINT64_C(0xff) && low_negative)) {
        *value = source_raw40;
        *clamped = false;
        return;
    }
    if ((source_raw40 & TI_C54X_U40_SIGN_BIT) != UINT64_C(0)) {
        *value = SAT_ACC_NEGATIVE_LIMIT;
    } else {
        *value = SAT_ACC_POSITIVE_LIMIT;
    }
    *clamped = true;
}

static ti_c54x_op_status sat_acc_compute(
    uint8_t source, const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40;
    ti_c54x_u40_t value;
    bool clamped;
    ti_c54x_op_effect staged = {0};

    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    source_raw40 = source == UINT8_C(0) ? input->accumulator_a_raw40
                                        : input->accumulator_b_raw40;
    if (source == UINT8_C(0) ? !input->accumulator_a_known
                             : !input->accumulator_b_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (source_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    sat_acc_saturate(source_raw40, &value, &clamped);
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = source;
    staged.payload.accumulator_write.value_raw40 = value;
    staged.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.destination_overflow_action =
        clamped ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                : TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_0;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_sat_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t source;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sat_acc_extract(instruction, &source);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sat_acc_compute(source, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_sat_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint8_t source;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    source = (uint8_t)operands->values[0].raw_value;
    if (operands->values[0].width_bits != UINT8_C(1) ||
        source > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return sat_acc_compute(source, input, effect);
}
