/* Module contract
 * Owns: checked and trusted syntax-6 dual-memory ADD/SUB MOD0 semantic
 * leaves.  The leaves stage one dual WORD16 read effect and, on pair
 * completion, one accumulator write with fixed-left-16 arithmetic, OVM
 * saturation, sticky destination overflow, and fixed-shift carry policy.
 * Does not own: catalog generation, operand layout definitions, address
 * resolution, memory/bus execution, architectural mutation, sequencing,
 * timing, observers, diagnostics, or allocation.  Borrowed instruction,
 * input, and effect objects are used for one call only.
 */

#include "op_dual_arithmetic.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "../../catalog/opcode_catalog.h"
#include "../../decode/operand_layout.h"
#include "../../../foundation/arithmetic40_addsub.h"
#include "../dispatch/trusted_semantics.h"

typedef struct ti_c54x_dual_arithmetic_operands {
    uint8_t destination;
    uint8_t x_ar;
    uint8_t y_ar;
} ti_c54x_dual_arithmetic_operands;

typedef enum ti_c54x_dual_arithmetic_kind {
    TI_C54X_DUAL_ARITHMETIC_ADD = 0,
    TI_C54X_DUAL_ARITHMETIC_SUB = 1
} ti_c54x_dual_arithmetic_kind;

static ti_c54x_op_status validate_metadata(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    ti_c54x_dual_arithmetic_operands *output)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value fields[3] = {{0U, 0U}, {0U, 0U}, {0U, 0U}};
    const ti_c54x_program_words_view words = {
        instruction == NULL ? NULL : instruction->bundle.program_words,
        instruction == NULL ? 0U : instruction->bundle.program_word_count};

    if (instruction == NULL || output == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_value != expected_value ||
        metadata->first_word_mask != UINT16_C(0xFECC) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_DUAL_DESTINATION_XAR_YAR ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_C_SXM_OVM ||
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
    for (uint8_t index = 0U; index < UINT8_C(3); ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &fields[index]) !=
            TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    if (fields[0].width_bits != UINT8_C(1) || fields[1].width_bits != UINT8_C(2) ||
        fields[2].width_bits != UINT8_C(2) || fields[0].raw_value > UINT64_C(1) ||
        fields[1].raw_value > UINT64_C(3) || fields[2].raw_value > UINT64_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    output->destination = (uint8_t)fields[0].raw_value;
    output->x_ar = (uint8_t)(fields[1].raw_value + UINT64_C(2));
    output->y_ar = (uint8_t)(fields[2].raw_value + UINT64_C(2));
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status validate_trusted(
    const ti_c54x_trusted_operand_values *trusted,
    ti_c54x_dual_arithmetic_operands *output)
{
    if (trusted == NULL || output == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (trusted->count != UINT8_C(3) ||
        trusted->values[0].width_bits != UINT8_C(1) ||
        trusted->values[1].width_bits != UINT8_C(2) ||
        trusted->values[2].width_bits != UINT8_C(2) ||
        trusted->values[0].raw_value > UINT64_C(1) ||
        trusted->values[1].raw_value > UINT64_C(3) ||
        trusted->values[2].raw_value > UINT64_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    output->destination = (uint8_t)trusted->values[0].raw_value;
    output->x_ar = (uint8_t)(trusted->values[1].raw_value + UINT64_C(2));
    output->y_ar = (uint8_t)(trusted->values[2].raw_value + UINT64_C(2));
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status validate_inputs(
    const ti_c54x_dual_arithmetic_operands *operands,
    const ti_c54x_op_input_snapshot *input)
{
    const uint8_t x_bit = (uint8_t)(UINT8_C(1) << operands->x_ar);
    const uint8_t y_bit = (uint8_t)(UINT8_C(1) << operands->y_ar);

    if (!input->status_c_known || !input->status_sxm_known ||
        !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_c > UINT8_C(1) || input->status_sxm > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if ((input->auxiliary_registers_known_mask & x_bit) == UINT8_C(0) ||
        (input->auxiliary_registers_known_mask & y_bit) == UINT8_C(0)) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_u40_t extend_word(uint16_t word, uint8_t sxm)
{
    ti_c54x_u40_t value = ti_c54x_u40_from_unsigned(word);

    if (sxm != UINT8_C(0) && (word & UINT16_C(0x8000)) != UINT16_C(0)) {
        value = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) | value;
    }
    return value;
}

static ti_c54x_op_status complete_values(
    const ti_c54x_dual_arithmetic_operands *operands,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word, uint16_t y_word,
    ti_c54x_dual_arithmetic_kind kind, ti_c54x_op_effect *next_effect)
{
    ti_c54x_u40_t x_value;
    ti_c54x_u40_t y_value;
    ti_c54x_u40_add_result add_result;
    ti_c54x_u40_sub_result sub_result;
    ti_c54x_u40_t value;
    ti_c54x_signed32_overflow_direction overflow;
    bool carry_or_no_borrow;
    ti_c54x_op_effect staged;
    ti_c54x_op_status status;

    status = validate_inputs(operands, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    x_value = ti_c54x_u40_logical_left_shift(
        extend_word(x_word, input->status_sxm), UINT64_C(16));
    y_value = ti_c54x_u40_logical_left_shift(
        extend_word(y_word, input->status_sxm), UINT64_C(16));
    if (kind == TI_C54X_DUAL_ARITHMETIC_ADD) {
        if (ti_c54x_u40_add_checked(x_value, y_value, &add_result) !=
            TI_C54X_ARITHMETIC40_ADDSUB_OK) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        value = add_result.value;
        carry_or_no_borrow = add_result.low32_carry;
        overflow = add_result.signed32_overflow;
    } else {
        if (ti_c54x_u40_sub_checked(x_value, y_value, &sub_result) !=
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
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = operands->destination,
            .value_raw40 = value,
            .status_delta = {
                .c_action =
                    kind == TI_C54X_DUAL_ARITHMETIC_ADD
                        ? (carry_or_no_borrow
                               ? TI_C54X_OP_STATUS_BIT_WRITE_1
                               : TI_C54X_OP_STATUS_BIT_PRESERVE)
                        : (carry_or_no_borrow
                               ? TI_C54X_OP_STATUS_BIT_PRESERVE
                               : TI_C54X_OP_STATUS_BIT_WRITE_0),
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .destination_overflow_action =
                    overflow == TI_C54X_SIGNED32_OVERFLOW_NONE
                        ? TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE
                        : TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1}}};
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status complete_add_dual(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word, uint16_t y_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_dual_arithmetic_operands operands;
    ti_c54x_op_status status = validate_metadata(
        instruction, TI_C54X_OPCODE_ID_ADD_DUAL_MOD0, UINT16_C(0xA000),
        &operands);
    if (status != TI_C54X_OP_STATUS_OK || next_effect == NULL || input == NULL) {
        return status != TI_C54X_OP_STATUS_OK ? status
                                               : TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    return complete_values(&operands, input, x_word, y_word,
                           TI_C54X_DUAL_ARITHMETIC_ADD, next_effect);
}

static ti_c54x_op_status complete_sub_dual(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t x_word, uint16_t y_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_dual_arithmetic_operands operands;
    ti_c54x_op_status status = validate_metadata(
        instruction, TI_C54X_OPCODE_ID_SUB_DUAL_MOD0, UINT16_C(0xA200),
        &operands);
    if (status != TI_C54X_OP_STATUS_OK || next_effect == NULL || input == NULL) {
        return status != TI_C54X_OP_STATUS_OK ? status
                                               : TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    return complete_values(&operands, input, x_word, y_word,
                           TI_C54X_DUAL_ARITHMETIC_SUB, next_effect);
}

static ti_c54x_op_status stage_memory(
    const ti_c54x_dual_arithmetic_operands *operands,
    ti_c54x_op_memory_complete_dual_fn completion, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (effect == NULL || operands == NULL || completion == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16,
                .dual = {.x_mode = UINT8_C(0),
                          .x_ar = operands->x_ar,
                          .y_mode = UINT8_C(0),
                          .y_ar = operands->y_ar,
                          .reserved_word0 = UINT32_C(0),
                          .reserved_word1 = UINT32_C(0),
                          .reserved_word2 = UINT32_C(0),
                          .reserved_word3 = UINT32_C(0)}},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0),
            .rmw_operation =
                (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            .complete_dual = completion}};

    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status checked_common(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_opcode_id expected_id, uint16_t expected_value,
    ti_c54x_op_memory_complete_dual_fn completion)
{
    ti_c54x_dual_arithmetic_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_metadata(instruction, expected_id, expected_value,
                               &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = validate_inputs(&operands, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return stage_memory(&operands, completion, effect);
}

static ti_c54x_op_status trusted_common(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    ti_c54x_op_memory_complete_dual_fn completion)
{
    ti_c54x_dual_arithmetic_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || trusted == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_trusted(trusted, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = validate_inputs(&operands, input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return stage_memory(&operands, completion, effect);
}

ti_c54x_op_status ti_c54x_op_add_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return checked_common(instruction, input, effect,
                          TI_C54X_OPCODE_ID_ADD_DUAL_MOD0, UINT16_C(0xA000),
                          complete_add_dual);
}

ti_c54x_op_status ti_c54x_op_sub_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return checked_common(instruction, input, effect,
                          TI_C54X_OPCODE_ID_SUB_DUAL_MOD0, UINT16_C(0xA200),
                          complete_sub_dual);
}

ti_c54x_op_status ti_c54x_trusted_op_add_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return trusted_common(instruction, operands, input, effect,
                          complete_add_dual);
}

ti_c54x_op_status ti_c54x_trusted_op_sub_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return trusted_common(instruction, operands, input, effect,
                          complete_sub_dual);
}
