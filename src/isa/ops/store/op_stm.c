/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for STM direct.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution, buses, CPU or architectural state,
 * sequencing, timing, observers, diagnostics, allocation,
 * logging, environment, or callbacks.
 */

#include "op_stm.h"

#include <stddef.h>
#include <stdint.h>

#include "../../decode/operand_layout.h"
#include "../../decode/operand_field.h"

static ti_c54x_op_status stm_validate_instruction(
    const ti_c54x_decoded_instruction *instruction, uint8_t *indirect,
    uint8_t *direct_dma, uint16_t *immediate)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value indirect_field;
    ti_c54x_operand_field_value address_field;
    ti_c54x_operand_field_value immediate_field;

    if (instruction == NULL || indirect == NULL || direct_dma == NULL ||
        immediate == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_STM);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_STM ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->id != TI_C54X_OPCODE_ID_STM ||
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
                &layout_view, &words, UINT8_C(0), &indirect_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &address_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &immediate_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            indirect_field.width_bits != UINT8_C(1) ||
            indirect_field.raw_value > UINT64_C(1) ||
            address_field.width_bits != UINT8_C(7) ||
            address_field.raw_value > UINT64_C(0x7F) ||
            immediate_field.width_bits != UINT8_C(16) ||
            immediate_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *indirect = (uint8_t)indirect_field.raw_value;
    *direct_dma = (uint8_t)address_field.raw_value;
    *immediate = (uint16_t)immediate_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static void stm_direct_memory_write(uint8_t direct_dma, uint16_t immediate,
                                    ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16,
                .direct_dma7 = direct_dma},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .value_word = immediate,
            .complete = NULL}};
}

ti_c54x_op_status ti_c54x_op_stm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t indirect;
    uint8_t direct_dma;
    uint16_t immediate;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = stm_validate_instruction(instruction, &indirect, &direct_dma,
                                      &immediate);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* The indirect Smem form is deferred: only I=0 direct composes. */
    if (indirect != UINT8_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    {
        ti_c54x_op_effect staged = {0};
        stm_direct_memory_write(direct_dma, immediate, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_stm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint8_t indirect;
    uint8_t direct_dma;
    uint16_t immediate;

    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(3) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(7) ||
        operands->values[1].raw_value > UINT64_C(0x7F) ||
        operands->values[2].width_bits != UINT8_C(16) ||
        operands->values[2].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    indirect = (uint8_t)operands->values[0].raw_value;
    direct_dma = (uint8_t)operands->values[1].raw_value;
    immediate = (uint16_t)operands->values[2].raw_value;
    /* The indirect Smem form is deferred: only I=0 direct composes. */
    if (indirect != UINT8_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    {
        ti_c54x_op_effect staged = {0};
        stm_direct_memory_write(direct_dma, immediate, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}
