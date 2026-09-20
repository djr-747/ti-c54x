/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for RPT Smem
 * syntax 1 (direct page-0 and standard-indirect MOD0--MOD3 reads
 * through the memory effect with a read-value REPEAT_START
 * completion loading RC = Smem through the shared immediate
 * core). Exotic indirect modes (mode4 above MOD3) defer as
 * INVALID_OPERAND. Neither path inspects input values or
 * knownness; address planning owns DP/AR downstream.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, RC state, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_rpt_smem.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "../../decode/operand_field.h"
#include "../../decode/operand_layout.h"
#include "op_rpt_immediate.h"

typedef struct rpt_smem_fields {
    uint8_t indirect;
    uint8_t addr7;
} rpt_smem_fields;

static ti_c54x_op_status rpt_smem_extract(
    const ti_c54x_decoded_instruction *instruction,
    rpt_smem_fields *fields)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    uint8_t index;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RPT_SMEM);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_RPT_SMEM ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->id != TI_C54X_OPCODE_ID_RPT_SMEM ||
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
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < UINT8_C(2); ++index) {
        const uint8_t want_width =
            index == UINT8_C(0) ? UINT8_C(1) : UINT8_C(7);
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != want_width ||
            field.raw_value > UINT64_C(0x7F)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        if (index == UINT8_C(0)) {
            fields->indirect = (uint8_t)field.raw_value;
        } else {
            fields->addr7 = (uint8_t)field.raw_value;
        }
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status rpt_smem_stage_read(
    const rpt_smem_fields *fields,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.load_t_with_read = false;
    staged.payload.memory.complete = complete;
    if (fields->indirect == UINT8_C(0)) {
        staged.payload.memory.address_spec.kind =
            TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16;
        staged.payload.memory.address_spec.direct_dma7 = fields->addr7;
    } else {
        const uint8_t mode4 = (uint8_t)(fields->addr7 >> 3);
        const uint8_t arf = (uint8_t)(fields->addr7 & UINT8_C(7));

        if (mode4 > UINT8_C(3)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        staged.payload.memory.address_spec.kind =
            TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
        staged.payload.memory.address_spec.mode =
            (ti_c54x_indirect_single_mode)mode4;
        staged.payload.memory.address_spec.arf = arf;
    }
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status rpt_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    rpt_smem_fields fields;
    ti_c54x_operand_field_value operand = {0};
    ti_c54x_op_status status;

    (void)input;
    if (instruction == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = rpt_smem_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)fields;
    operand.raw_value = read_value_word;
    operand.width_bits = UINT8_C(16);
    return ti_c54x_op_rpt_immediate_from_field(&operand, next_effect);
}

ti_c54x_op_status ti_c54x_op_rpt_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    rpt_smem_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = rpt_smem_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return rpt_smem_stage_read(&fields, rpt_smem_complete, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_rpt_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    rpt_smem_fields fields;

    (void)instruction;
    (void)input;
    if (operands == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(7) ||
        operands->values[1].raw_value > UINT64_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields.indirect = (uint8_t)operands->values[0].raw_value;
    fields.addr7 = (uint8_t)operands->values[1].raw_value;
    return rpt_smem_stage_read(&fields, rpt_smem_complete, effect);
}
