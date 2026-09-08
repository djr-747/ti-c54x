/* Module contract
 * Owns: pure standard-indirect WORD16 STL memory-write effects over one
 * selected canonical accumulator low word, one ARF operand, and one MOD
 * operand.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "op_stl_smem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_store_indexed_common.h"

static void stl_memory_write(uint8_t arf, uint8_t mode, uint16_t value,
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

static ti_c54x_op_status stl_values(
    uint8_t source, uint8_t arf, uint8_t mode,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40;
    bool source_known;
    ti_c54x_op_effect staged = {0};

    if (source > UINT8_C(1) || arf > UINT8_C(7) || mode > UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
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

    stl_memory_write(arf, mode, (uint16_t)source_raw40, &staged);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_stl_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value arf_field = {0U, 0U};
    ti_c54x_operand_field_value mode_field = {0U, 0U};

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_STL_SMEM);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_STL_SMEM ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_ACCUMULATORS ||
        instruction->id != TI_C54X_OPCODE_ID_STL_SMEM ||
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
                &layout_view, &words, UINT8_C(0), &source_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &arf_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &mode_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            source_field.width_bits != UINT8_C(1) ||
            arf_field.width_bits != UINT8_C(3) ||
            source_field.raw_value > UINT64_C(1) ||
            arf_field.raw_value > UINT64_C(7) ||
            mode_field.width_bits != UINT8_C(2) ||
            mode_field.raw_value > UINT64_C(3)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    return stl_values((uint8_t)source_field.raw_value,
                      (uint8_t)arf_field.raw_value,
                      (uint8_t)mode_field.raw_value, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_stl_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(3) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(3) ||
        operands->values[1].raw_value > UINT64_C(7) ||
        operands->values[2].width_bits != UINT8_C(2) ||
        operands->values[2].raw_value > UINT64_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return stl_values((uint8_t)operands->values[0].raw_value,
                      (uint8_t)operands->values[1].raw_value,
                      (uint8_t)operands->values[2].raw_value, input, effect);
}

static ti_c54x_op_status stl_indexed_values(
    const ti_c54x_store_indexed_operands *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40;
    bool source_known;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->source > UINT8_C(1) || operands->arf > UINT8_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (operands->source == UINT8_C(0)) {
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
    {
        ti_c54x_op_effect staged = {0};
        ti_c54x_store_stage_indexed_write(
            operands->arf, operands->mode, (uint16_t)source_raw40, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status stl_indexed_checked(
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
        instruction, expected_id,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return stl_indexed_values(&operands, input, effect);
}

static ti_c54x_op_status stl_indexed_trusted(
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
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
        TI_C54X_INPUT_RESOURCES_ACCUMULATORS, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return stl_indexed_values(&operands, input, effect);
}

ti_c54x_op_status ti_c54x_op_stl_smem_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return stl_indexed_checked(instruction, input, TI_C54X_OPCODE_ID_STL_SMEM_MOD5,
                               (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, effect);
}

ti_c54x_op_status ti_c54x_op_stl_smem_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return stl_indexed_checked(instruction, input, TI_C54X_OPCODE_ID_STL_SMEM_MOD6,
                               (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_stl_smem_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return stl_indexed_trusted(
        instruction, operands, input, TI_C54X_OPCODE_ID_STL_SMEM_MOD5,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_stl_smem_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    return stl_indexed_trusted(
        instruction, operands, input, TI_C54X_OPCODE_ID_STL_SMEM_MOD6,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, effect);
}
