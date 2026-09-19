/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for MVMD absolute.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution, buses, CPU or architectural state,
 * sequencing, timing, observers, diagnostics, allocation,
 * logging, environment, or callbacks.
 */

#include "op_mvmd.h"

#include <stddef.h>
#include <stdint.h>

#include "../../decode/operand_layout.h"
#include "../../decode/operand_field.h"

static ti_c54x_op_status mvmd_validate_instruction(
    const ti_c54x_decoded_instruction *instruction, uint8_t *mmr,
    uint16_t *address)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value mmr_field;
    ti_c54x_operand_field_value address_field;

    if (instruction == NULL || mmr == NULL || address == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MVMD);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_MVMD ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->id != TI_C54X_OPCODE_ID_MVMD ||
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
                &layout_view, &words, UINT8_C(0), &mmr_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &address_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            mmr_field.width_bits != UINT8_C(5) ||
            mmr_field.raw_value > UINT64_C(0x1F) ||
            address_field.width_bits != UINT8_C(16) ||
            address_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *mmr = (uint8_t)mmr_field.raw_value;
    *address = (uint16_t)address_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static void mvmd_absolute_memory_write(uint8_t mmr, uint16_t address,
                                       ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind =
                    TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16,
                .absolute_data = {
                    .address = address,
                    .mmr_sidecar_kind = (uint8_t)
                        TI_C54X_OP_MEMORY_MMR_SIDECAR_LOAD_WITH_WRITE,
                    .mmr_sidecar_number = mmr}},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .complete = NULL}};
}

ti_c54x_op_status ti_c54x_op_mvmd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t mmr;
    uint16_t address;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mvmd_validate_instruction(instruction, &mmr, &address);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }

    {
        ti_c54x_op_effect staged = {0};
        mvmd_absolute_memory_write(mmr, address, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_mvmd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint8_t mmr;
    uint16_t address;

    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(5) ||
        operands->values[0].raw_value > UINT64_C(0x1F) ||
        operands->values[1].width_bits != UINT8_C(16) ||
        operands->values[1].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    mmr = (uint8_t)operands->values[0].raw_value;
    address = (uint16_t)operands->values[1].raw_value;

    {
        ti_c54x_op_effect staged = {0};
        mvmd_absolute_memory_write(mmr, address, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}
