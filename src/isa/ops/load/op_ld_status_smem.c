/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for fixed-MOD2 WORD16
 * indirect loads into the DP and ASM status fields. The checked leaf
 * validates the per-ID catalog/decode contract and stages a generic memory
 * read; its static completion masks the copied WORD16 and stages the status
 * field write. The trusted leaf consumes constructor-validated operands.
 * Neither path interprets input values or knownness.
 * Does not own: opcode inventory, generic decode, AR resolution or mutation,
 * buses, CPU/state mutation, sequencing, timing, observers, allocation,
 * logging, environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained; failed
 * calls leave the caller-owned effect unchanged.
 */

#include "op_ld_status_smem.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct ld_status_smem_descriptor {
    ti_c54x_opcode_id id;
    uint16_t first_word_value;
    uint16_t first_word_mask;
    ti_c54x_repeatability_class repeatability;
    ti_c54x_op_status_field_target target;
    uint16_t completion_mask;
} ld_status_smem_descriptor;

static const ld_status_smem_descriptor ld_status_smem_dp = {
    TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD2,
    UINT16_C(0x4690),
    UINT16_C(0xFFF8),
    TI_C54X_REPEATABILITY_INELIGIBLE,
    TI_C54X_OP_STATUS_FIELD_TARGET_DP,
    UINT16_C(0x01FF)};

static const ld_status_smem_descriptor ld_status_smem_asm = {
    TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD2,
    UINT16_C(0x3290),
    UINT16_C(0xFFF8),
    TI_C54X_REPEATABILITY_UNRESOLVED,
    TI_C54X_OP_STATUS_FIELD_TARGET_ASM,
    UINT16_C(0x001F)};

static const ld_status_smem_descriptor *ld_status_smem_descriptor_for_id(
    ti_c54x_opcode_id id)
{
    switch (id) {
    case TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD2:
        return &ld_status_smem_dp;
    case TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD2:
        return &ld_status_smem_asm;
    default:
        return NULL;
    }
}

static ti_c54x_op_status ld_status_smem_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    const ld_status_smem_descriptor **descriptor,
    uint8_t *arf)
{
    const ld_status_smem_descriptor *expected;
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value arf_field = {0U, 0U};

    if (instruction == NULL || descriptor == NULL || arf == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    expected = ld_status_smem_descriptor_for_id(expected_id);
    if (expected == NULL || instruction->id != expected_id) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    metadata = ti_c54x_opcode_metadata_for_id(expected->id);
    if (metadata == NULL || metadata->id != expected->id ||
        metadata->first_word_value != expected->first_word_value ||
        metadata->first_word_mask != expected->first_word_mask ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_ARF ||
        metadata->repeatability != expected->repeatability ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
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
                &layout_view, &words, UINT8_C(0), &arf_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            arf_field.width_bits != UINT8_C(3) ||
            arf_field.raw_value > UINT64_C(7)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *descriptor = expected;
    *arf = (uint8_t)arf_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static void ld_status_smem_memory_read(uint8_t arf, ti_c54x_op_effect *effect);

static ti_c54x_op_status ld_status_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    const ld_status_smem_descriptor *descriptor;
    ti_c54x_op_effect staged;
    uint8_t arf;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_status_smem_validate_instruction(
        instruction, instruction->id, &descriptor, &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)arf;
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE,
        .payload.status_field_write = {
            .target = descriptor->target,
            .raw_value = (uint16_t)(read_value_word &
                                    descriptor->completion_mask)}};
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static void ld_status_smem_memory_read(uint8_t arf, ti_c54x_op_effect *effect)
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
            .complete = ld_status_smem_complete}};
}

static ti_c54x_op_status ld_status_smem_checked_for_id(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id,
    ti_c54x_op_effect *effect)
{
    const ld_status_smem_descriptor *descriptor;
    ti_c54x_op_status status;
    uint8_t arf;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_status_smem_validate_instruction(
        instruction, expected_id, &descriptor, &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)descriptor;
    {
        ti_c54x_op_effect staged = {0};
        ld_status_smem_memory_read(arf, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_ld_dp_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return ld_status_smem_checked_for_id(
        instruction, input, TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD2, effect);
}

ti_c54x_op_status ti_c54x_op_ld_asm_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return ld_status_smem_checked_for_id(
        instruction, input, TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD2, effect);
}

static ti_c54x_op_status ld_status_smem_trusted_for_id(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_opcode_id expected_id,
    ti_c54x_op_effect *effect)
{
    const ld_status_smem_descriptor *descriptor;
    ti_c54x_op_status status;
    uint8_t arf;

    if (instruction == NULL || operands == NULL || input == NULL ||
        effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_status_smem_validate_instruction(
        instruction, expected_id, &descriptor, &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)descriptor;
    (void)arf;
    if (operands->count != UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(3) ||
        operands->values[0].raw_value > UINT64_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    {
        ti_c54x_op_effect staged = {0};
        ld_status_smem_memory_read((uint8_t)operands->values[0].raw_value,
                                   &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ld_dp_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return ld_status_smem_trusted_for_id(
        instruction, operands, input, TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD2,
        effect);
}

ti_c54x_op_status ti_c54x_trusted_op_ld_asm_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return ld_status_smem_trusted_for_id(
        instruction, operands, input, TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD2,
        effect);
}
