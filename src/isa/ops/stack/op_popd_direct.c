/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for `POPD Smem`
 * (direct page-0 dma plus MOD0--MOD3 single-indirect Smem).
 * Does not own: opcode inventory, operand-layout
 * definitions, SP state, buses, CPU or architectural state,
 * sequencing, timing, observers, diagnostics, allocation,
 * logging, environment, or callbacks.
 */

#include "op_popd_direct.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../../decode/operand_layout.h"
#include "../../decode/operand_field.h"

static ti_c54x_op_status popd_values(
    uint8_t indirect, uint8_t addr,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (addr > UINT8_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    /* Unknown SP rejects transactionally (CALA precedent); the
     * root re-pins liveness against live state. */
    if (!input->stack_pointer_known) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    memset(&staged, 0, sizeof(staged));
    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16;
    if (indirect == UINT8_C(0)) {
        staged.payload.memory.address_spec.stack_pair.indirect = UINT8_C(0);
        staged.payload.memory.address_spec.stack_pair.smem_dma7 = addr;
        staged.payload.memory.address_spec.stack_pair.smem_mode = UINT8_C(0);
        staged.payload.memory.address_spec.stack_pair.smem_arf = UINT8_C(0);
    } else {
        /* Single-indirect Smem (class 17B): the cookie splits
         * into mode4 = addr >> 3 with arf = addr & 7. The DATA
         * leg writes, so MOD0--MOD3 all retire. */
        const uint8_t mode4 = (uint8_t)(addr >> 3);
        const uint8_t arf = (uint8_t)(addr & UINT8_C(7));

        if (mode4 > UINT8_C(3)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        staged.payload.memory.address_spec.stack_pair.indirect = UINT8_C(1);
        staged.payload.memory.address_spec.stack_pair.smem_mode = mode4;
        staged.payload.memory.address_spec.stack_pair.smem_arf = arf;
        staged.payload.memory.address_spec.stack_pair.smem_dma7 = UINT8_C(0);
    }
    staged.payload.memory.access =
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.rmw_operation =
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE;
    staged.payload.memory.complete = NULL;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status popd_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value addr_field = {0U, 0U};

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_POPD_DIRECT);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_POPD_DIRECT ||
        metadata->first_word_value != UINT16_C(0x8B00) ||
        metadata->first_word_mask != UINT16_C(0xFF00) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_STACK_POINTER ||
        instruction->id != TI_C54X_OPCODE_ID_POPD_DIRECT ||
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
                &layout_view, &words, UINT8_C(1), &addr_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            indirect_field.width_bits != UINT8_C(1) ||
            addr_field.width_bits != UINT8_C(7) ||
            addr_field.raw_value > UINT64_C(0x7F)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    return popd_values((uint8_t)indirect_field.raw_value,
                       (uint8_t)addr_field.raw_value, input, effect);
}

ti_c54x_op_status ti_c54x_op_popd_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return popd_checked(instruction, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_popd_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(0x01) ||
        operands->values[1].width_bits != UINT8_C(7) ||
        operands->values[1].raw_value > UINT64_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return popd_values((uint8_t)operands->values[0].raw_value,
                       (uint8_t)operands->values[1].raw_value, input,
                       effect);
}
