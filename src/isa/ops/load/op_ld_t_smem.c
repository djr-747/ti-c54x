/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for standard-indirect
 * WORD16 loads into the temporary T register. The checked leaf validates the
 * catalog/decode contract, consumes MOD0--MOD2, and stages a generic
 * memory-read effect; its static completion validates the copied instruction
 * before staging the T-register write. The trusted leaf consumes
 * constructor-validated operands. Neither path interprets input values or
 * knownness. Does not own: opcode inventory, generic decode, AR resolution or
 * mutation, buses, CPU/state mutation, sequencing, timing, observers,
 * allocation, logging, environment, or diagnostics. Borrowed
 * instruction/input/effect objects are used for one call only and no pointer
 * is retained; failed calls leave the caller-owned effect unchanged.
 */

#include "op_ld_t_smem.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status ld_t_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

static ti_c54x_op_status ld_t_smem_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    uint8_t *arf,
    uint8_t *mode)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value arf_field = {0U, 0U};
    ti_c54x_operand_field_value mode_field = {0U, 0U};

    if (instruction == NULL || arf == NULL || mode == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_T_SMEM);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_LD_T_SMEM ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value & (uint16_t)~metadata->first_word_mask) !=
            UINT16_C(0) ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_ARF_MOD ||
        metadata->repeatability < TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->id != TI_C54X_OPCODE_ID_LD_T_SMEM ||
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

static void ld_t_smem_memory_read(uint8_t arf, uint8_t mode,
                                  ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16,
                .mode = (ti_c54x_indirect_single_mode)mode,
                .arf = arf},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .write_value_word = UINT16_C(0),
            .complete = ld_t_smem_complete}};
}

static ti_c54x_op_status ld_t_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    uint8_t arf;
    uint8_t mode;
    ti_c54x_op_status status;
    ti_c54x_op_effect staged;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_t_smem_validate_instruction(instruction, &arf, &mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (mode == (uint8_t)TI_C54X_INDIRECT_SINGLE_MOD3) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    (void)arf;

    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_T_REGISTER_WRITE,
        .payload.t_register_write = {.raw_value = read_value_word}};
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_ld_t_smem(
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
    status = ld_t_smem_validate_instruction(instruction, &arf, &mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (mode == (uint8_t)TI_C54X_INDIRECT_SINGLE_MOD3) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    {
        ti_c54x_op_effect staged = {0};
        ld_t_smem_memory_read(arf, mode, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ld_t_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t arf;
    uint8_t mode;

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
    arf = (uint8_t)operands->values[0].raw_value;
    mode = (uint8_t)operands->values[1].raw_value;
    if (mode == (uint8_t)TI_C54X_INDIRECT_SINGLE_MOD3) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    {
        ti_c54x_op_effect staged = {0};
        ld_t_smem_memory_read(arf, mode, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}
