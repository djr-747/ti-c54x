/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the one-word fixed-MOD2
 * WORD16 store of the copied T register. The checked leaf validates the
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

static void st_t_smem_memory_write(uint8_t arf, uint16_t value,
                                   ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16,
                .mode = TI_C54X_INDIRECT_SINGLE_MOD2,
                .arf = arf},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .write_value_word = value,
            .complete = NULL}};
}

static ti_c54x_op_status st_t_smem_values(
    uint8_t arf, const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (arf > UINT8_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (!input->t_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }

    st_t_smem_memory_write(arf, input->t_register_raw16, &staged);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_t_smem_validate_instruction(
    const ti_c54x_decoded_instruction *instruction, uint8_t *arf)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value arf_field = {0U, 0U};

    if (instruction == NULL || arf == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_ST_T_SMEM_MOD2);
    if (metadata == NULL ||
        metadata->id != TI_C54X_OPCODE_ID_ST_T_SMEM_MOD2 ||
        metadata->first_word_value != UINT16_C(0x8C90) ||
        metadata->first_word_mask != UINT16_C(0xFFF8) ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_ARF ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_T_REGISTER ||
        instruction->id != TI_C54X_OPCODE_ID_ST_T_SMEM_MOD2 ||
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
            arf_field.width_bits != UINT8_C(3) ||
            arf_field.raw_value > UINT64_C(7)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *arf = (uint8_t)arf_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_st_t_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t arf;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = st_t_smem_validate_instruction(instruction, &arf);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_t_smem_values(arf, input, effect);
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
    if (operands->count != UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(3) ||
        operands->values[0].raw_value > UINT64_C(7)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    return st_t_smem_values((uint8_t)operands->values[0].raw_value, input,
                            effect);
}
