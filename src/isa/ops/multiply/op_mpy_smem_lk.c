/*
 * Module contract
 *
 * Owns: checked and trusted operand validation plus value-owned
 * MEMORY-effect staging for the MPY Smem,#lk,dst syntax-3 form:
 * destination select D, indirect flag I, and 7-bit Smem address
 * from word 0 with the copied 16-bit immediate from word 1. The
 * read carries the read-phase T load and a read-value STORE
 * completion of Smem times signed lk into dst through the
 * multiply core with FRCT shift, OVM saturation, and sticky
 * OVdst; C and TC are preserved.
 * Does not own: catalog generation, decode, address resolution,
 * buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, callbacks, or status
 * mutation. No raw first-word fields are parsed here; operand
 * extraction is delegated to the decoder-owned layout.
 */

#include "op_mpy_smem_lk.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_field.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_multiply.h"

typedef struct mpy_smem_lk_fields {
    uint8_t destination;
    uint8_t indirect;
    uint8_t addr7;
    uint16_t immediate;
} mpy_smem_lk_fields;

static ti_c54x_op_status mpy_smem_lk_extract(
    const ti_c54x_decoded_instruction *instruction,
    mpy_smem_lk_fields *fields)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    /* Operand order: destination, indirect flag, Smem address,
     * immediate. */
    static const uint8_t widths[4] = {UINT8_C(1), UINT8_C(1), UINT8_C(7),
                                      UINT8_C(16)};
    static const uint64_t limits[4] = {UINT64_C(1), UINT64_C(1),
                                       UINT64_C(0x7F), UINT64_C(0xFFFF)};
    uint8_t index;
    uint64_t got[4] = {UINT64_C(0), UINT64_C(0), UINT64_C(0),
                       UINT64_C(0)};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MPY_SMEM_LK);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_MPY_SMEM_LK ||
        metadata->first_word_value != UINT16_C(0x6200) ||
        metadata->first_word_mask != UINT16_C(0xFE00) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7_EXTENSION16 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_ELIGIBLE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_STATUS_FRCT_OVM ||
        instruction->id != TI_C54X_OPCODE_ID_MPY_SMEM_LK ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
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
    for (index = UINT8_C(0); index < UINT8_C(4); ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != widths[index] ||
            field.raw_value > limits[index]) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        got[index] = field.raw_value;
    }
    fields->destination = (uint8_t)got[0];
    fields->indirect = (uint8_t)got[1];
    fields->addr7 = (uint8_t)got[2];
    fields->immediate = (uint16_t)got[3];
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mpy_smem_lk_validate_inputs(
    const ti_c54x_op_input_snapshot *input)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!input->status_frct_known || !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_frct > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mpy_smem_lk_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    mpy_smem_lk_fields fields;
    ti_c54x_op_status status;
    ti_c54x_multiply_operands operands;
    ti_c54x_multiply_result result;
    ti_c54x_multiply_status multiply_status;
    ti_c54x_op_effect staged = {0};

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mpy_smem_lk_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = mpy_smem_lk_validate_inputs(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    operands.t_raw16 = read_value_word;
    operands.t_known = true;
    operands.multiplicand_raw16 = fields.immediate;
    operands.multiplicand_known = true;
    operands.accumulator_raw40 = UINT64_C(0);
    operands.accumulator_known = false;
    operands.frct = input->status_frct;
    operands.frct_known = true;
    operands.ovm = input->status_ovm;
    operands.ovm_known = true;
    operands.unsigned_operands = false;
    operands.unsigned_t_only = false;
    operands.rounding = false;
    operands.select = TI_C54X_MULTIPLY_STORE;
    multiply_status = ti_c54x_multiply_accumulate(&operands, &result);
    if (multiply_status == TI_C54X_MULTIPLY_UNKNOWN) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (multiply_status != TI_C54X_MULTIPLY_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields.destination;
    staged.payload.accumulator_write.value_raw40 = result.accumulator_raw40;
    staged.payload.accumulator_write.status_delta.c_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.destination_overflow_action =
        result.overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                        : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status mpy_smem_lk_stage_read(
    const mpy_smem_lk_fields *fields,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.load_t_with_read = true;
    staged.payload.memory.complete = mpy_smem_lk_complete;
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

ti_c54x_op_status ti_c54x_op_mpy_smem_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    mpy_smem_lk_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mpy_smem_lk_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = mpy_smem_lk_validate_inputs(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return mpy_smem_lk_stage_read(&fields, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_mpy_smem_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    mpy_smem_lk_fields fields;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(4) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(1) ||
        operands->values[1].raw_value > UINT64_C(1) ||
        operands->values[2].width_bits != UINT8_C(7) ||
        operands->values[2].raw_value > UINT64_C(0x7F) ||
        operands->values[3].width_bits != UINT8_C(16) ||
        operands->values[3].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields.destination = (uint8_t)operands->values[0].raw_value;
    fields.indirect = (uint8_t)operands->values[1].raw_value;
    fields.addr7 = (uint8_t)operands->values[2].raw_value;
    fields.immediate = (uint16_t)operands->values[3].raw_value;
    status = mpy_smem_lk_validate_inputs(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return mpy_smem_lk_stage_read(&fields, effect);
}
