/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the one-word standard-
 * indirect WORD16 store of the copied T register. The checked leaf validates the
 * catalog/decode contract and extracts ARF; both paths consume only copied T
 * knownness/value and stage one generic memory-write effect. Borrowed
 * instruction, operand, input, and effect objects are used for one call only;
 * no pointer is retained and failed calls leave the caller effect unchanged.
 * Does not own: opcode inventory, operand layouts, T state or collection,
 * address resolution, buses, CPU/state mutation, sequencing, timing,
 * observers, diagnostics, allocation, logging, or environment.
 */

#include "op_st_t_smem.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_store_indexed_common.h"
#include "op_store_circular_common.h"

static void st_t_smem_memory_write(uint8_t arf, uint8_t mode, uint16_t value,
                                   ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16,
                .mode = (ti_c54x_indirect_single_mode)mode,
                .arf = arf},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .write_value_word = value,
            .complete = NULL}};
}

static ti_c54x_op_status st_t_smem_values(
    uint8_t arf, uint8_t mode, const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (arf > UINT8_C(7) || mode > UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (!input->t_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }

    st_t_smem_memory_write(arf, mode, input->t_register_raw16, &staged);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_t_smem_validate_instruction(
    const ti_c54x_decoded_instruction *instruction, uint8_t *arf,
    uint8_t *mode)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value arf_field = {0U, 0U};
    ti_c54x_operand_field_value mode_field = {0U, 0U};

    if (instruction == NULL || arf == NULL || mode == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_ST_T_SMEM);
    if (metadata == NULL ||
        metadata->id != TI_C54X_OPCODE_ID_ST_T_SMEM ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_ARF_MOD ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_T_REGISTER ||
        instruction->id != TI_C54X_OPCODE_ID_ST_T_SMEM ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
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
                &layout_view, &words, UINT8_C(0), &arf_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &mode_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            arf_field.width_bits != UINT8_C(3) ||
            arf_field.raw_value > UINT64_C(7) ||
            mode_field.width_bits != UINT8_C(2) ||
            mode_field.raw_value > UINT64_C(3)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *arf = (uint8_t)arf_field.raw_value;
    *mode = (uint8_t)mode_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_st_t_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t arf;
    uint8_t mode;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = st_t_smem_validate_instruction(instruction, &arf, &mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_t_smem_values(arf, mode, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_st_t_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(3) ||
        operands->values[0].raw_value > UINT64_C(7) ||
        operands->values[1].width_bits != UINT8_C(2) ||
        operands->values[1].raw_value > UINT64_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    return st_t_smem_values((uint8_t)operands->values[0].raw_value,
                            (uint8_t)operands->values[1].raw_value, input,
                            effect);
}

static ti_c54x_op_status st_t_smem_indexed_values(
    const ti_c54x_store_indexed_operands *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->arf > UINT8_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (!input->t_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    {
        ti_c54x_op_effect staged = {0};
        ti_c54x_store_stage_indexed_write(
            operands->arf, operands->mode, input->t_register_raw16, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_t_smem_indexed_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id, uint8_t expected_mode,
    ti_c54x_op_effect *effect)
{
    ti_c54x_store_indexed_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_store_validate_indexed_instruction(
        instruction, expected_id, TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
        TI_C54X_INPUT_RESOURCES_T_REGISTER, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_t_smem_indexed_values(&operands, input, effect);
}

static ti_c54x_op_status st_t_smem_indexed_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id, uint8_t expected_mode,
    ti_c54x_op_effect *effect)
{
    ti_c54x_store_indexed_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || trusted_operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_store_validate_indexed_trusted_operands(
        instruction, trusted_operands, expected_id,
        TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
        TI_C54X_INPUT_RESOURCES_T_REGISTER, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_t_smem_indexed_values(&operands, input, effect);
}

static ti_c54x_op_status st_t_smem_circular_values(
    const ti_c54x_store_circular_operands *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->arf > UINT8_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (!input->t_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    {
        ti_c54x_op_effect staged = {0};
        ti_c54x_store_stage_circular_write(
            operands->arf, operands->mode, input->t_register_raw16, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_t_smem_circular_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_op_effect *effect)
{
    ti_c54x_store_circular_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_store_validate_circular_instruction(
        instruction, expected_id, TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
        TI_C54X_INPUT_RESOURCES_T_REGISTER, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_t_smem_circular_values(&operands, input, effect);
}

static ti_c54x_op_status st_t_smem_circular_trusted(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_op_effect *effect)
{
    ti_c54x_store_circular_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || trusted_operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_store_validate_circular_trusted_operands(
        instruction, trusted_operands, expected_id,
        TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
        TI_C54X_INPUT_RESOURCES_T_REGISTER, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_t_smem_circular_values(&operands, input, effect);
}

ti_c54x_op_status ti_c54x_op_st_t_smem_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_t_smem_indexed_checked(
        instruction, input, TI_C54X_OPCODE_ID_ST_T_SMEM_MOD5,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, effect);
}

ti_c54x_op_status ti_c54x_op_st_t_smem_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_t_smem_indexed_checked(
        instruction, input, TI_C54X_OPCODE_ID_ST_T_SMEM_MOD6,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_st_t_smem_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_t_smem_indexed_trusted(
        instruction, operands, input, TI_C54X_OPCODE_ID_ST_T_SMEM_MOD5,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_st_t_smem_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_t_smem_indexed_trusted(
        instruction, operands, input, TI_C54X_OPCODE_ID_ST_T_SMEM_MOD6,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, effect);
}

ti_c54x_op_status ti_c54x_op_st_t_smem_mod8(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_t_smem_circular_checked(
        instruction, input, TI_C54X_OPCODE_ID_ST_T_SMEM_MOD8,
        TI_C54X_INDIRECT_CIRCULAR_MOD8, effect);
}

ti_c54x_op_status ti_c54x_op_st_t_smem_mod10(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_t_smem_circular_checked(
        instruction, input, TI_C54X_OPCODE_ID_ST_T_SMEM_MOD10,
        TI_C54X_INDIRECT_CIRCULAR_MOD10, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_st_t_smem_mod8(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_t_smem_circular_trusted(
        instruction, operands, input, TI_C54X_OPCODE_ID_ST_T_SMEM_MOD8,
        TI_C54X_INDIRECT_CIRCULAR_MOD8, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_st_t_smem_mod10(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return st_t_smem_circular_trusted(
        instruction, operands, input, TI_C54X_OPCODE_ID_ST_T_SMEM_MOD10,
        TI_C54X_INDIRECT_CIRCULAR_MOD10, effect);
}
