/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for SQURA Smem
 * and SQURS Smem, sharing one ADD/SUBTRACT-selected completion
 * core. Both rows stage direct page-0 and standard-indirect
 * MOD0--MOD3 reads through the memory effect with read-phase T
 * load; the read-value completion runs (src) plus/minus (Smem) x
 * (Smem) into the S-selected accumulator through the multiply
 * core with FRCT/OVM handling and no rounding. Exotic indirect
 * modes (mode4 above MOD3) defer as INVALID_OPERAND.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include "op_squr_acc.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_field.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_multiply.h"

typedef struct squr_acc_fields {
    uint8_t select;
    uint8_t indirect;
    uint8_t addr7;
} squr_acc_fields;

static const uint8_t squr_acc_widths[3] = {UINT8_C(1), UINT8_C(1),
                                           UINT8_C(7)};

static ti_c54x_op_status squr_acc_extract_fields(
    const ti_c54x_operand_field_value *fields, squr_acc_fields *output)
{
    if (fields == NULL || output == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (fields[0].width_bits != squr_acc_widths[0] ||
        fields[1].width_bits != squr_acc_widths[1] ||
        fields[2].width_bits != squr_acc_widths[2] ||
        fields[0].raw_value > UINT64_C(1) ||
        fields[1].raw_value > UINT64_C(1) ||
        fields[2].raw_value > UINT64_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    output->select = (uint8_t)fields[0].raw_value;
    output->indirect = (uint8_t)fields[1].raw_value;
    output->addr7 = (uint8_t)fields[2].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status squr_acc_extract(
    ti_c54x_opcode_id id, const ti_c54x_decoded_instruction *instruction,
    squr_acc_fields *fields)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value unpacked[3] = {{0U, 0U},
                                              {0U, 0U},
                                              {0U, 0U}};
    ti_c54x_program_words_view words;
    uint8_t index;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(id);
    if (metadata == NULL || metadata->id != id ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7 ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->repeatability != TI_C54X_REPEATABILITY_ELIGIBLE ||
        instruction->id != id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
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
    for (index = UINT8_C(0); index < UINT8_C(3); ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index,
                &unpacked[index]) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    return squr_acc_extract_fields(unpacked, fields);
}

static ti_c54x_op_status squr_acc_src_accumulator(
    const ti_c54x_op_input_snapshot *input, uint8_t select,
    uint64_t *accumulator_raw40)
{
    if (select == UINT8_C(0)) {
        if (!input->accumulator_a_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
        *accumulator_raw40 = input->accumulator_a_raw40;
    } else {
        if (!input->accumulator_b_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
        *accumulator_raw40 = input->accumulator_b_raw40;
    }
    if (*accumulator_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status squr_acc_validate_inputs(
    const ti_c54x_op_input_snapshot *input)
{
    if (!input->status_frct_known || !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_frct > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status squr_acc_stage_read(
    const squr_acc_fields *fields,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.load_t_with_read = true;
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

static ti_c54x_op_status squr_acc_complete_select(
    ti_c54x_multiply_select select,
    const squr_acc_fields *fields,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_op_status status;
    uint64_t src_raw40;
    ti_c54x_multiply_operands operands;
    ti_c54x_multiply_result result;
    ti_c54x_multiply_status multiply_status;
    ti_c54x_op_effect staged = {0};

    status = squr_acc_src_accumulator(input, fields->select, &src_raw40);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* Square: the Smem word feeds both multiplier inputs. */
    operands.t_raw16 = read_value_word;
    operands.t_known = true;
    operands.multiplicand_raw16 = read_value_word;
    operands.multiplicand_known = true;
    operands.accumulator_raw40 = src_raw40;
    operands.accumulator_known = true;
    operands.frct = input->status_frct;
    operands.frct_known = true;
    operands.ovm = input->status_ovm;
    operands.ovm_known = true;
    operands.unsigned_operands = false;
    operands.rounding = false;
    operands.select = select;
    multiply_status = ti_c54x_multiply_accumulate(&operands, &result);
    if (multiply_status == TI_C54X_MULTIPLY_UNKNOWN) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (multiply_status != TI_C54X_MULTIPLY_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields->select;
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

static ti_c54x_op_status squra_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    squr_acc_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = squr_acc_extract(TI_C54X_OPCODE_ID_SQURA_SMEM, instruction,
                              &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = squr_acc_validate_inputs(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return squr_acc_complete_select(TI_C54X_MULTIPLY_ADD, &fields, input,
                                    read_value_word, next_effect);
}

static ti_c54x_op_status squrs_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    squr_acc_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = squr_acc_extract(TI_C54X_OPCODE_ID_SQURS_SMEM, instruction,
                              &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = squr_acc_validate_inputs(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return squr_acc_complete_select(TI_C54X_MULTIPLY_SUBTRACT, &fields, input,
                                    read_value_word, next_effect);
}

static ti_c54x_op_status squr_acc_checked(
    ti_c54x_opcode_id id, ti_c54x_op_memory_complete_fn complete,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    squr_acc_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = squr_acc_extract(id, instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = squr_acc_validate_inputs(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return squr_acc_stage_read(&fields, complete, effect);
}

static ti_c54x_op_status squr_acc_trusted(
    ti_c54x_op_memory_complete_fn complete,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    squr_acc_fields fields;
    ti_c54x_op_status status;
    ti_c54x_operand_field_value unpacked[3];
    uint8_t index;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    for (index = UINT8_C(0); index < UINT8_C(3); ++index) {
        unpacked[index].width_bits = operands->values[index].width_bits;
        unpacked[index].raw_value = operands->values[index].raw_value;
    }
    status = squr_acc_extract_fields(unpacked, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = squr_acc_validate_inputs(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return squr_acc_stage_read(&fields, complete, effect);
}

ti_c54x_op_status ti_c54x_op_squra_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return squr_acc_checked(TI_C54X_OPCODE_ID_SQURA_SMEM, squra_smem_complete,
                            instruction, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_squra_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;
    return squr_acc_trusted(squra_smem_complete, operands, input, effect);
}

ti_c54x_op_status ti_c54x_op_squrs_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return squr_acc_checked(TI_C54X_OPCODE_ID_SQURS_SMEM, squrs_smem_complete,
                            instruction, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_squrs_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;
    return squr_acc_trusted(squrs_smem_complete, operands, input, effect);
}
