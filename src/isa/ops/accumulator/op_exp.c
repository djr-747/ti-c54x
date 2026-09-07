/* Module contract
 * Owns: pure EXP accumulator semantics over copied canonical raw40 inputs,
 * including source selection and staged T-register effects.
 * Does not own: accumulator or T-register storage, operand-layout definitions,
 * decode tables, effect application, sequencing, CPU lifecycle, timing,
 * repeat/interrupt policy, or diagnostics.
 */

#include "op_exp.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static uint16_t ti_c54x_exp_encode(ti_c54x_u40_t source_raw40)
{
    uint64_t sign;
    uint8_t bit_index;
    uint8_t redundant_bits = UINT8_C(0);
    int32_t exponent;

    if (source_raw40 == UINT64_C(0)) {
        return UINT16_C(0);
    }

    sign = (source_raw40 & TI_C54X_U40_SIGN_BIT) != UINT64_C(0)
               ? UINT64_C(1)
               : UINT64_C(0);
    bit_index = UINT8_C(38);
    for (;;) {
        const uint64_t current = (source_raw40 >> bit_index) & UINT64_C(1);

        if (current != sign) {
            break;
        }
        ++redundant_bits;
        if (bit_index == UINT8_C(0)) {
            break;
        }
        --bit_index;
    }

    exponent = (int32_t)redundant_bits - INT32_C(8);
    if (exponent < INT32_C(0)) {
        return (uint16_t)(UINT16_C(0) - (uint16_t)(-exponent));
    }
    return (uint16_t)exponent;
}

static ti_c54x_op_status exp_values(
    uint8_t source,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};
    ti_c54x_u40_t source_raw40;
    bool source_known;

    if (source == UINT8_C(0)) {
        source_raw40 = input->accumulator_a_raw40;
        source_known = input->accumulator_a_known;
    } else {
        source_raw40 = input->accumulator_b_raw40;
        source_known = input->accumulator_b_known;
    }
    if (!source_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (source_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    staged.kind = TI_C54X_OP_EFFECT_T_REGISTER_WRITE;
    staged.payload.t_register_write.raw_value =
        ti_c54x_exp_encode(source_raw40);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_exp_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_EXP_ACC);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_EXP_ACC ||
        metadata->first_word_value != UINT16_C(0xF48E) ||
        metadata->first_word_mask != UINT16_C(0xFEFF) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_ACCUMULATORS ||
        instruction->id != TI_C54X_OPCODE_ID_EXP_ACC ||
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
                &layout_view, &words, UINT8_C(0), &source_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            source_field.width_bits != UINT8_C(1) ||
            source_field.raw_value > UINT64_C(1)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    return exp_values((uint8_t)source_field.raw_value, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_exp_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;
    return exp_values((uint8_t)operands->values[0].raw_value,
                      input, effect);
}
