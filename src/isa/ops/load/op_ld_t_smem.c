/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for standard-indirect
 * WORD16 loads into the temporary T register, including circular MOD8/MOD10.
 * The checked leaf validates the
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

#include "op_ld_smem_common.h"

static ti_c54x_op_status ld_t_smem_indexed_complete(
    ti_c54x_opcode_id expected_id,
    uint8_t expected_mode,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_indexed_instruction(
        instruction, expected_id, TI_C54X_INPUT_RESOURCES_NONE,
        TI_C54X_OPERAND_LAYOUT_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_T_REGISTER_WRITE,
        .payload.t_register_write = {.raw_value = read_value_word}};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_t_smem_mod5_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    return ld_t_smem_indexed_complete(
        TI_C54X_OPCODE_ID_LD_T_SMEM_MOD5,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, instruction, input,
        read_value_word, next_effect);
}

static ti_c54x_op_status ld_t_smem_mod6_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    return ld_t_smem_indexed_complete(
        TI_C54X_OPCODE_ID_LD_T_SMEM_MOD6,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, instruction, input,
        read_value_word, next_effect);
}

static ti_c54x_op_status ld_t_smem_checked_indexed(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id,
    uint8_t expected_mode,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_indexed_instruction(
        instruction, expected_id, TI_C54X_INPUT_RESOURCES_NONE,
        TI_C54X_OPERAND_LAYOUT_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_indexed_memory_read(&operands, expected_mode, complete,
                                              effect);
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_t_smem_trusted_indexed(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id,
    uint8_t expected_mode,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || trusted_operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_indexed_trusted_operands(
        instruction, trusted_operands, expected_id,
        TI_C54X_INPUT_RESOURCES_NONE, TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
        expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_indexed_memory_read(&operands, expected_mode, complete,
                                              effect);
    return TI_C54X_OP_STATUS_OK;
}

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

ti_c54x_op_status ti_c54x_op_ld_t_smem_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return ld_t_smem_checked_indexed(
        instruction, input, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD5,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, ld_t_smem_mod5_complete,
        effect);
}

ti_c54x_op_status ti_c54x_op_ld_t_smem_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return ld_t_smem_checked_indexed(
        instruction, input, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD6,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, ld_t_smem_mod6_complete,
        effect);
}

ti_c54x_op_status ti_c54x_trusted_op_ld_t_smem_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return ld_t_smem_trusted_indexed(
        instruction, operands, input, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD5,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD5, ld_t_smem_mod5_complete,
        effect);
}

ti_c54x_op_status ti_c54x_trusted_op_ld_t_smem_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return ld_t_smem_trusted_indexed(
        instruction, operands, input, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD6,
        (uint8_t)TI_C54X_INDIRECT_INDEXED_MOD6, ld_t_smem_mod6_complete,
        effect);
}

static ti_c54x_op_status ld_t_smem_circular_complete(
    ti_c54x_opcode_id expected_id, ti_c54x_indirect_circular_mode expected_mode,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_circular_instruction(
        instruction, expected_id, TI_C54X_INPUT_RESOURCES_NONE,
        TI_C54X_OPERAND_LAYOUT_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_T_REGISTER_WRITE,
        .payload.t_register_write = {.raw_value = read_value_word}};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_t_smem_checked_circular(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_opcode_id expected_id,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_op_memory_complete_fn complete, ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_circular_instruction(
        instruction, expected_id, TI_C54X_INPUT_RESOURCES_NONE,
        TI_C54X_OPERAND_LAYOUT_ARF_MOD4, expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_circular_memory_read(
        &operands, expected_mode, complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_t_smem_trusted_circular(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_opcode_id expected_id,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_op_memory_complete_fn complete, ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || trusted_operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_circular_trusted_operands(
        instruction, trusted_operands, expected_id,
        TI_C54X_INPUT_RESOURCES_NONE, TI_C54X_OPERAND_LAYOUT_ARF_MOD4,
        expected_mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_circular_memory_read(
        &operands, expected_mode, complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

#define DEFINE_LD_T_CIRCULAR_CALLBACK(suffix, id, mode)                         \
    static ti_c54x_op_status ld_t_smem_##suffix##_circular_complete(            \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,        \
        ti_c54x_op_effect *next_effect)                                          \
    {                                                                             \
        return ld_t_smem_circular_complete(id, mode, instruction, input,         \
                                           read_value_word, next_effect);         \
    }

#define DEFINE_LD_T_CIRCULAR_WRAPPER(suffix, id, mode)                          \
    ti_c54x_op_status ti_c54x_op_ld_t_smem_##suffix(                            \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)       \
    {                                                                             \
        return ld_t_smem_checked_circular(                                       \
            instruction, input, id, mode,                                        \
            ld_t_smem_##suffix##_circular_complete, effect);                     \
    }                                                                             \
    ti_c54x_op_status ti_c54x_trusted_op_ld_t_smem_##suffix(                    \
        const ti_c54x_decoded_instruction *instruction,                          \
        const ti_c54x_trusted_operand_values *operands,                          \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)        \
    {                                                                             \
        return ld_t_smem_trusted_circular(                                       \
            instruction, operands, input, id, mode,                              \
            ld_t_smem_##suffix##_circular_complete, effect);                     \
    }

DEFINE_LD_T_CIRCULAR_CALLBACK(mod8, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD8,
                              TI_C54X_INDIRECT_CIRCULAR_MOD8)
DEFINE_LD_T_CIRCULAR_CALLBACK(mod10, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD10,
                              TI_C54X_INDIRECT_CIRCULAR_MOD10)
DEFINE_LD_T_CIRCULAR_WRAPPER(mod8, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD8,
                             TI_C54X_INDIRECT_CIRCULAR_MOD8)
DEFINE_LD_T_CIRCULAR_WRAPPER(mod10, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD10,
                             TI_C54X_INDIRECT_CIRCULAR_MOD10)

#undef DEFINE_LD_T_CIRCULAR_WRAPPER
#undef DEFINE_LD_T_CIRCULAR_CALLBACK
