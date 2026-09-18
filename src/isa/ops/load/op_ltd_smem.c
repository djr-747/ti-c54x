/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the row-less LTD Smem
 * form (direct page-0 and standard-indirect MOD0--MOD3 Smem DATA reads with
 * the Smem delay write-back of the Smem word to DATA Smem+1 and a
 * T-register-write completion carrying the read word). The checked leaf
 * gates the UNKNOWN instruction id, the INDIRECT_ADDR7 operand layout,
 * input resources NONE, one program word matching (word & 0xFF00) == 0x4C00,
 * and the 1-bit indirect plus 7-bit address field widths. Exotic indirect
 * modes (mode4 above MOD3) defer as INVALID_OPERAND. The trusted leaf
 * consumes constructor-validated operands through the same staging core,
 * and the shared completion re-validates the copied instruction before
 * staging the T-register write. No accumulator or status inputs are read
 * and no status bits are affected.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution or mutation, buses, CPU/state mutation, sequencing, timing,
 * observers, allocation, logging, environment, or diagnostics. Borrowed
 * instruction/input/effect objects are used for one call only and no pointer
 * is retained; failed calls leave the caller-owned effect unchanged.
 */

#include "op_ltd_smem.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_field.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct ltd_smem_fields {
    uint8_t indirect;
    uint8_t addr7;
} ltd_smem_fields;

static ti_c54x_op_status ltd_smem_extract(
    const ti_c54x_decoded_instruction *instruction,
    ltd_smem_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    uint8_t index;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_LTD_SMEM ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7 ||
        instruction->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFF00)) != UINT16_C(0x4C00)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7, UINT8_C(1),
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < UINT8_C(2); ++index) {
        const uint8_t want_width =
            index == UINT8_C(0) ? UINT8_C(1) : UINT8_C(7);
        const uint64_t want_max =
            index == UINT8_C(0) ? UINT64_C(1) : UINT64_C(0x7F);
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != want_width || field.raw_value > want_max) {
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

static ti_c54x_op_status ltd_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

static ti_c54x_op_status ltd_smem_stage_read(
    const ltd_smem_fields *fields, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.complete = ltd_smem_complete;
    staged.payload.memory.write_smem_delay = true;
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

static ti_c54x_op_status ltd_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ltd_smem_fields fields;
    ti_c54x_op_status status;
    ti_c54x_op_effect staged;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ltd_smem_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (fields.indirect != UINT8_C(0) &&
        (uint8_t)(fields.addr7 >> 3) > UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged.kind = TI_C54X_OP_EFFECT_T_REGISTER_WRITE;
    staged.payload.t_register_write.raw_value = read_value_word;
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_ltd_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ltd_smem_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ltd_smem_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ltd_smem_stage_read(&fields, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_ltd_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ltd_smem_fields fields;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
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
    return ltd_smem_stage_read(&fields, effect);
}
