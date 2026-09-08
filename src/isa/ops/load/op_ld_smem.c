/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for fixed-MOD2 WORD16
 * indirect loads into accumulator A or B, including the unshifted,
 * SXM-dependent logical-left-shift-16, and T-register-shift forms. Each
 * checked leaf validates its catalog/decode contract and stages a generic
 * memory-read effect; its static
 * completion validates the copied instruction and input before staging the
 * accumulator write. The trusted leaves consume constructor-validated
 * operands while retaining SXM input validation.
 * Does not own: opcode inventory, generic decode, AR resolution or mutation,
 * buses, CPU/state mutation, sequencing, timing, observers, allocation,
 * logging, environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained; failed calls
 * leave the caller-owned effect unchanged.
 */

#include "op_ld_smem.h"

#include <stdbool.h>
#include <stddef.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status ld_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

static ti_c54x_op_status ld_smem_shift16_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

static ti_c54x_op_status ld_smem_ts_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

static ti_c54x_op_status ld_smem_validate_sxm(
    const ti_c54x_op_input_snapshot *input)
{
    if (!input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_sxm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_smem_validate_instruction_for_id(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    uint8_t *destination,
    uint8_t *arf)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value arf_field = {0U, 0U};

    if (instruction == NULL || destination == NULL || arf == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value & (uint16_t)~metadata->first_word_mask) !=
            UINT16_C(0) ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF ||
        metadata->repeatability < TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != expected_resources ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count ||
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
                &layout_view, &words, UINT8_C(0), &destination_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &arf_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            destination_field.width_bits != UINT8_C(1) ||
            destination_field.raw_value > UINT64_C(1) ||
            arf_field.width_bits != UINT8_C(3) ||
            arf_field.raw_value > UINT64_C(7)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *destination = (uint8_t)destination_field.raw_value;
    *arf = (uint8_t)arf_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_smem_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    uint8_t *destination,
    uint8_t *arf)
{
    return ld_smem_validate_instruction_for_id(
        instruction, TI_C54X_OPCODE_ID_LD_SMEM_MOD2,
        TI_C54X_INPUT_RESOURCES_STATUS_SXM, destination, arf);
}

static ti_c54x_op_status ld_smem_shift16_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    uint8_t *destination,
    uint8_t *arf)
{
    return ld_smem_validate_instruction_for_id(
        instruction, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD2,
        TI_C54X_INPUT_RESOURCES_STATUS_SXM, destination, arf);
}

static ti_c54x_op_status ld_smem_ts_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    uint8_t *destination,
    uint8_t *arf)
{
    return ld_smem_validate_instruction_for_id(
        instruction, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD2,
        TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, destination, arf);
}

static void ld_smem_memory_read(
    uint8_t arf,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16,
                .mode = TI_C54X_INDIRECT_SINGLE_MOD2,
                .arf = arf},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .write_value_word = UINT16_C(0),
            .complete = complete}};
}

static void ld_smem_stage_memory_read(
    uint8_t arf,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    ld_smem_memory_read(arf, complete, &staged);
    *effect = staged;
}

static ti_c54x_op_status ld_smem_complete_values(
    uint8_t destination,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    uint8_t shift,
    bool arithmetic_right,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_op_status input_status = ld_smem_validate_sxm(input);
    ti_c54x_op_effect staged;
    ti_c54x_u40_t value_raw40;

    if (input_status != TI_C54X_OP_STATUS_OK) {
        return input_status;
    }

    value_raw40 = ti_c54x_u40_from_unsigned(read_value_word);
    if (input->status_sxm != UINT8_C(0) &&
        (read_value_word & UINT16_C(0x8000)) != UINT16_C(0)) {
        value_raw40 = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                      (uint64_t)read_value_word;
    }
    value_raw40 = arithmetic_right
                      ? ti_c54x_u40_arithmetic_right_shift(value_raw40, shift)
                      : ti_c54x_u40_logical_left_shift(value_raw40, shift);
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = value_raw40,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE}}};
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_smem_ts_validate_input(
    const ti_c54x_op_input_snapshot *input)
{
    const ti_c54x_op_status sxm_status = ld_smem_validate_sxm(input);
    const uint8_t shift_bits =
        (uint8_t)(input->t_register_raw16 & UINT16_C(0x3F));

    if (sxm_status != TI_C54X_OP_STATUS_OK) {
        return sxm_status;
    }
    if (!input->t_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (shift_bits >= UINT8_C(0x20) && shift_bits <= UINT8_C(0x2F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_smem_ts_complete_values(
    uint8_t destination,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    const ti_c54x_op_status input_status = ld_smem_ts_validate_input(input);
    const uint8_t shift_bits =
        (uint8_t)(input->t_register_raw16 & UINT16_C(0x3F));

    if (input_status != TI_C54X_OP_STATUS_OK) {
        return input_status;
    }
    return ld_smem_complete_values(
        destination, input, read_value_word, shift_bits <= UINT8_C(31)
                                                 ? shift_bits
                                                 : (uint8_t)(UINT8_C(64) -
                                                             shift_bits),
        shift_bits > UINT8_C(31), next_effect);
}

static ti_c54x_op_status ld_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    uint8_t destination;
    uint8_t arf;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_smem_validate_instruction(instruction, &destination, &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)arf;
    return ld_smem_complete_values(destination, input, read_value_word, 0U,
                                   false, next_effect);
}

static ti_c54x_op_status ld_smem_shift16_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    uint8_t destination;
    uint8_t arf;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_smem_shift16_validate_instruction(instruction, &destination,
                                                   &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)arf;
    return ld_smem_complete_values(destination, input, read_value_word, 16U,
                                   false, next_effect);
}

static ti_c54x_op_status ld_smem_ts_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    uint8_t destination;
    uint8_t arf;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_smem_ts_validate_instruction(instruction, &destination, &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)arf;
    return ld_smem_ts_complete_values(destination, input, read_value_word,
                                      next_effect);
}

ti_c54x_op_status ti_c54x_op_ld_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t destination;
    uint8_t arf;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_smem_validate_instruction(instruction, &destination, &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)destination;
    status = ld_smem_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ld_smem_stage_memory_read(arf, ld_smem_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ld_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(3) ||
        operands->values[1].raw_value > UINT64_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = ld_smem_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ld_smem_stage_memory_read((uint8_t)operands->values[1].raw_value,
                              ld_smem_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_ld_smem_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t destination;
    uint8_t arf;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_smem_shift16_validate_instruction(instruction, &destination,
                                                   &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)destination;
    status = ld_smem_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ld_smem_stage_memory_read(arf, ld_smem_shift16_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_ld_smem_ts(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t destination;
    uint8_t arf;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_smem_ts_validate_instruction(instruction, &destination, &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)destination;
    status = ld_smem_ts_validate_input(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ld_smem_stage_memory_read(arf, ld_smem_ts_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ld_smem_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(3) ||
        operands->values[1].raw_value > UINT64_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = ld_smem_validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ld_smem_stage_memory_read((uint8_t)operands->values[1].raw_value,
                              ld_smem_shift16_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ld_smem_ts(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(3) ||
        operands->values[1].raw_value > UINT64_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = ld_smem_ts_validate_input(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ld_smem_stage_memory_read((uint8_t)operands->values[1].raw_value,
                              ld_smem_ts_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}
