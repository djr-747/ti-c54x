/* Module contract
 * Owns: pure row-less SFTA accumulator semantics over copied canonical
 * raw40 inputs, including source/destination selection, sign-extended
 * five-bit shifts, SXM-dependent right shifts, OVM saturation, and staged
 * accumulator/C/overflow effects.
 * Does not own: accumulator storage, status mutation, catalog rows, decode
 * tables, dispatch binding, sequencing, CPU lifecycle, timing,
 * repeat/interrupt policy, or diagnostics.
 */

#include "op_sfta_acc.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"

#define TI_C54X_SFTA_ACC_FIRST_WORD_VALUE UINT16_C(0xF460)
#define TI_C54X_SFTA_ACC_FIRST_WORD_MASK UINT16_C(0xFCE0)

typedef struct sfta_acc_operands {
    uint8_t source;
    uint8_t destination;
    uint8_t shift_raw5;
} sfta_acc_operands;

static ti_c54x_op_status sfta_acc_validate_inputs(
    const ti_c54x_op_input_snapshot *input, uint8_t source)
{
    bool source_known;
    ti_c54x_u40_t source_raw40;

    if (!input->status_sxm_known || !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_sxm > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    source_known = source == UINT8_C(0) ? input->accumulator_a_known
                                        : input->accumulator_b_known;
    if (!source_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    source_raw40 = source == UINT8_C(0) ? input->accumulator_a_raw40
                                        : input->accumulator_b_raw40;
    if (source_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sfta_acc_values(
    uint8_t source, uint8_t destination, uint8_t shift_raw5,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40 = source == UINT8_C(0)
                                     ? input->accumulator_a_raw40
                                     : input->accumulator_b_raw40;
    ti_c54x_u40_t value_raw40;
    uint8_t c_value;
    bool overflow = false;
    ti_c54x_op_effect staged;
    int shift = (int)shift_raw5;

    if ((shift & 0x10) != 0) {
        shift -= 32;
    }
    if (shift < 0) {
        const unsigned count = (unsigned)(-shift);
        const bool sign =
            (source_raw40 & TI_C54X_U40_SIGN_BIT) != UINT64_C(0);
        uint64_t shifted = source_raw40 >> count;
        c_value = (uint8_t)((source_raw40 >> (count - 1U)) & UINT64_C(1));
        if (input->status_sxm != UINT8_C(0) && sign) {
            shifted |= TI_C54X_U40_MASK ^ (TI_C54X_U40_MASK >> count);
        }
        value_raw40 = ti_c54x_u40_mask(shifted);
    } else if (shift == 0) {
        c_value = (uint8_t)((source_raw40 >> UINT64_C(39)) & UINT64_C(1));
        value_raw40 = source_raw40;
    } else {
        const unsigned count = (unsigned)shift;
        const uint64_t wrapped =
            (source_raw40 << count) & TI_C54X_U40_MASK;
        const unsigned width_bits = 8U + count;
        const uint64_t field = source_raw40 >> (32U - count);
        const uint8_t reference =
            (uint8_t)((source_raw40 >> (31U - count)) & UINT64_C(1));
        const uint64_t intact =
            reference != UINT8_C(0)
                ? (UINT64_C(1) << width_bits) - UINT64_C(1)
                : UINT64_C(0);
        c_value =
            (uint8_t)((source_raw40 >> (39U - count)) & UINT64_C(1));
        overflow = field != intact;
        if (overflow && input->status_ovm != UINT8_C(0)) {
            const bool negative =
                ((wrapped >> UINT64_C(31)) & UINT64_C(1)) != UINT64_C(0);
            value_raw40 = negative ? UINT64_C(0xFF80000000)
                                   : UINT64_C(0x007FFFFFFF);
        } else {
            value_raw40 = ti_c54x_u40_mask(wrapped);
        }
    }

    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = value_raw40,
            .status_delta = {
                .c_action = c_value != UINT8_C(0)
                    ? TI_C54X_OP_STATUS_BIT_WRITE_1
                    : TI_C54X_OP_STATUS_BIT_WRITE_0,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .destination_overflow_action =
                    overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                             : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE}}};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sfta_acc_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    sfta_acc_operands *operands)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value shift_field = {0U, 0U};

    if (instruction == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_SFTA_ACC ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5 ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   TI_C54X_SFTA_ACC_FIRST_WORD_MASK) !=
            TI_C54X_SFTA_ACC_FIRST_WORD_VALUE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5,
            UINT8_C(1), &layout_view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &source_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &destination_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &shift_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            source_field.width_bits != UINT8_C(1) ||
            source_field.raw_value > UINT64_C(1) ||
            destination_field.width_bits != UINT8_C(1) ||
            destination_field.raw_value > UINT64_C(1) ||
            shift_field.width_bits != UINT8_C(5) ||
            shift_field.raw_value > UINT64_C(0x1F)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    operands->source = (uint8_t)source_field.raw_value;
    operands->destination = (uint8_t)destination_field.raw_value;
    operands->shift_raw5 = (uint8_t)shift_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sfta_acc_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands,
    sfta_acc_operands *decoded_operands)
{
    if (operands == NULL || decoded_operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(3) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(1) ||
        operands->values[1].raw_value > UINT64_C(1) ||
        operands->values[2].width_bits != UINT8_C(5) ||
        operands->values[2].raw_value > UINT64_C(0x1F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    decoded_operands->source = (uint8_t)operands->values[0].raw_value;
    decoded_operands->destination =
        (uint8_t)operands->values[1].raw_value;
    decoded_operands->shift_raw5 = (uint8_t)operands->values[2].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_sfta_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    sfta_acc_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sfta_acc_validate_instruction(instruction, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = sfta_acc_validate_inputs(input, operands.source);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sfta_acc_values(operands.source, operands.destination,
                           operands.shift_raw5, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_sfta_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    sfta_acc_operands decoded_operands;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status =
        sfta_acc_validate_trusted_operands(operands, &decoded_operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = sfta_acc_validate_inputs(input, decoded_operands.source);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sfta_acc_values(decoded_operands.source,
                           decoded_operands.destination,
                           decoded_operands.shift_raw5, input, effect);
}
