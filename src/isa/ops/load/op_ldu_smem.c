/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for fixed-MOD2 WORD16
 * indirect unsigned loads into accumulator A or B. The checked leaf validates
 * the catalog/decode contract and stages a generic memory-read effect; its
 * static completion zero-extends the copied WORD16 into a canonical 40-bit
 * accumulator write. The trusted leaf consumes constructor-validated
 * operands. Neither path observes or interprets SXM. Does not own: opcode
 * inventory, generic decode, AR resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging, environment,
 * or diagnostics. Borrowed instruction/input/effect objects are used for one
 * call only and no pointer is retained; failed calls leave the caller-owned
 * effect unchanged.
 */

#include "op_ldu_smem.h"

#include <stddef.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status ldu_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

static ti_c54x_op_status ldu_smem_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
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

    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_LDU_SMEM_MOD2);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_LDU_SMEM_MOD2 ||
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
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->id != TI_C54X_OPCODE_ID_LDU_SMEM_MOD2 ||
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

static void ldu_smem_memory_read(uint8_t arf, ti_c54x_op_effect *effect)
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
            .complete = ldu_smem_complete}};
}

static ti_c54x_op_status ldu_smem_complete_values(
    uint8_t destination,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    const ti_c54x_op_effect staged = {
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = (ti_c54x_u40_t)read_value_word,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE}}};

    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ldu_smem_complete(
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
    status = ldu_smem_validate_instruction(instruction, &destination, &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)arf;
    return ldu_smem_complete_values(destination, read_value_word, next_effect);
}

ti_c54x_op_status ti_c54x_op_ldu_smem(
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
    status = ldu_smem_validate_instruction(instruction, &destination, &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)destination;
    {
        ti_c54x_op_effect staged = {0};
        ldu_smem_memory_read(arf, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ldu_smem(
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
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(3) ||
        operands->values[1].raw_value > UINT64_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    {
        ti_c54x_op_effect staged = {0};
        ldu_smem_memory_read((uint8_t)operands->values[1].raw_value, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}
