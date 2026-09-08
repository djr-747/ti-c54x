/* Module contract
 * Owns: pure checked/trusted ADD_ACC_SHIFT and SUB_ACC_SHIFT semantics over
 * copied raw40 accumulators, signed five-bit shifts, SXM/OVM handling, and
 * staged accumulator/C/overflow effects.
 * Does not own: catalog lookup generation, accumulator/status mutation, CPU
 * sequencing, timing, buses, or diagnostics.
 */

#include "op_arithmetic.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40_addsub.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef enum ti_c54x_arithmetic_kind {
    TI_C54X_ARITHMETIC_ADD = 0,
    TI_C54X_ARITHMETIC_SUB = 1
} ti_c54x_arithmetic_kind;

static int decode_shift(uint8_t raw_shift)
{
    return (raw_shift & UINT8_C(0x10)) != 0U
               ? (int)raw_shift - 32
               : (int)raw_shift;
}

static ti_c54x_u40_t shift_source(ti_c54x_u40_t source, int shift,
                                  uint8_t sxm)
{
    if (shift > 0) {
        return ti_c54x_u40_mask(source << (unsigned)shift);
    }
    if (shift < 0) {
        const unsigned count = (unsigned)(-shift);
        return sxm == UINT8_C(0)
                   ? ti_c54x_u40_mask(source >> count)
                   : ti_c54x_u40_arithmetic_right_shift(source, count);
    }
    return source;
}

static ti_c54x_op_status arithmetic_values(
    uint8_t source, uint8_t destination, uint8_t raw_shift,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect, ti_c54x_arithmetic_kind kind)
{
    ti_c54x_u40_t source_raw40;
    ti_c54x_u40_t destination_raw40;
    ti_c54x_u40_t shifted_source;
    ti_c54x_u40_add_result add_result;
    ti_c54x_u40_sub_result sub_result;
    ti_c54x_op_effect staged = {0};
    ti_c54x_signed32_overflow_direction overflow;
    ti_c54x_u40_t value;
    bool carry_or_no_borrow;

    if (!input->status_sxm_known || !input->status_ovm_known ||
        input->status_sxm > UINT8_C(1) || input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    source_raw40 = source == UINT8_C(0) ? input->accumulator_a_raw40
                                         : input->accumulator_b_raw40;
    destination_raw40 = destination == UINT8_C(0)
                            ? input->accumulator_a_raw40
                            : input->accumulator_b_raw40;
    if ((source == UINT8_C(0) ? !input->accumulator_a_known
                              : !input->accumulator_b_known) ||
        (source != destination &&
         (destination == UINT8_C(0) ? !input->accumulator_a_known
                                    : !input->accumulator_b_known))) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (source_raw40 > TI_C54X_U40_MASK ||
        (source != destination && destination_raw40 > TI_C54X_U40_MASK)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    shifted_source = shift_source(source_raw40, decode_shift(raw_shift),
                                   input->status_sxm);
    if (kind == TI_C54X_ARITHMETIC_ADD) {
        if (ti_c54x_u40_add_checked(destination_raw40, shifted_source,
                                    &add_result) !=
            TI_C54X_ARITHMETIC40_ADDSUB_OK) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        value = add_result.value;
        carry_or_no_borrow = add_result.low32_carry;
        overflow = add_result.signed32_overflow;
    } else {
        if (ti_c54x_u40_sub_checked(destination_raw40, shifted_source,
                                    &sub_result) !=
            TI_C54X_ARITHMETIC40_ADDSUB_OK) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        value = sub_result.value;
        carry_or_no_borrow = sub_result.low32_no_borrow;
        overflow = sub_result.signed32_overflow;
    }

    if (input->status_ovm != UINT8_C(0)) {
        if (overflow == TI_C54X_SIGNED32_OVERFLOW_POSITIVE) {
            value = UINT64_C(0x007fffffff);
        } else if (overflow == TI_C54X_SIGNED32_OVERFLOW_NEGATIVE) {
            value = UINT64_C(0xff80000000);
        }
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = destination;
    staged.payload.accumulator_write.value_raw40 = value;
    staged.payload.accumulator_write.status_delta.c_action =
        carry_or_no_borrow ? TI_C54X_OP_STATUS_BIT_WRITE_1
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

static ti_c54x_op_status arithmetic_common(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id, uint16_t expected_first_word,
    ti_c54x_arithmetic_kind kind)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value fields[3] = {{0U, 0U}, {0U, 0U}, {0U, 0U}};
    const ti_c54x_program_words_view words = {
        instruction == NULL ? NULL : instruction->bundle.program_words,
        instruction == NULL ? 0U : instruction->bundle.program_word_count};

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_value != expected_first_word ||
        metadata->first_word_mask != UINT16_C(0xFCE0) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    for (uint8_t index = 0U; index < 3U; ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &fields[index]) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    if (fields[0].width_bits != UINT8_C(1) ||
        fields[1].width_bits != UINT8_C(1) ||
        fields[2].width_bits != UINT8_C(5) || fields[0].raw_value > 1U ||
        fields[1].raw_value > 1U || fields[2].raw_value > 0x1FU) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return arithmetic_values((uint8_t)fields[0].raw_value,
                             (uint8_t)fields[1].raw_value,
                             (uint8_t)fields[2].raw_value, input, effect,
                             kind);
}

static ti_c54x_op_status arithmetic_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_arithmetic_kind kind)
{
    (void)instruction;
    return arithmetic_values((uint8_t)operands->values[0].raw_value,
                             (uint8_t)operands->values[1].raw_value,
                             (uint8_t)operands->values[2].raw_value, input,
                             effect, kind);
}

ti_c54x_op_status ti_c54x_op_add_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return arithmetic_common(instruction, input, effect,
                             TI_C54X_OPCODE_ID_ADD_ACC_SHIFT,
                             UINT16_C(0xF400), TI_C54X_ARITHMETIC_ADD);
}

ti_c54x_op_status ti_c54x_op_sub_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return arithmetic_common(instruction, input, effect,
                             TI_C54X_OPCODE_ID_SUB_ACC_SHIFT,
                             UINT16_C(0xF420), TI_C54X_ARITHMETIC_SUB);
}

ti_c54x_op_status ti_c54x_trusted_op_add_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return arithmetic_trusted(instruction, operands, input, effect,
                              TI_C54X_ARITHMETIC_ADD);
}

ti_c54x_op_status ti_c54x_trusted_op_sub_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return arithmetic_trusted(instruction, operands, input, effect,
                              TI_C54X_ARITHMETIC_SUB);
}
