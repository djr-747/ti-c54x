/* Module contract
 * Owns: pure checked/trusted ADDS Smem semantics (dst(src) = src40 plus
 * zero-extended smem16) over direct page-0 and standard-indirect
 * MOD0--MOD3 reads through the memory effect, with OVM saturation plus
 * sticky OVdst and the sibling ADD C rule (C written from the bit-32
 * carry, TC preserved). Smem is always zero-extended regardless of SXM;
 * SXM is not consumed and never gates the op. Exotic indirect modes
 * (mode4 above MOD3) defer as INVALID_OPERAND. No catalog rows exist
 * for this form yet, so the checked entry keys on UNKNOWN id, the
 * shared SELECT_INDIRECT_ADDR7 operand layout, a single program word,
 * and the fixed first-word field; dispatch binding follows when the
 * rows land.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_adds_smem.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "foundation/arithmetic40_addsub.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/memory/memory_effect.h"

#define ADDS_SMEM_FIRST_WORD_MASK UINT16_C(0xFE00)
#define ADDS_SMEM_FIRST_WORD_VALUE UINT16_C(0x0200)

typedef struct adds_smem_fields {
    uint8_t select;
    uint8_t indirect;
    uint8_t addr7;
} adds_smem_fields;

static ti_c54x_op_status adds_smem_extract(
    const ti_c54x_decoded_instruction *instruction,
    adds_smem_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    uint8_t index;
    uint8_t got[3] = {0U, 0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_ADDS_SMEM ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7 ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   ADDS_SMEM_FIRST_WORD_MASK) !=
            ADDS_SMEM_FIRST_WORD_VALUE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7, UINT8_C(1),
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < UINT8_C(3); ++index) {
        const uint8_t want_width =
            index == UINT8_C(2) ? UINT8_C(7) : UINT8_C(1);
        const uint64_t want_max =
            index == UINT8_C(2) ? UINT64_C(0x7F) : UINT64_C(1);
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != want_width ||
            field.raw_value > want_max) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        got[index] = (uint8_t)field.raw_value;
    }
    fields->select = got[0];
    fields->indirect = got[1];
    fields->addr7 = got[2];
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status adds_smem_validate_inputs(
    const ti_c54x_op_input_snapshot *input, uint8_t select)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if ((select == UINT8_C(0) ? !input->accumulator_a_known
                             : !input->accumulator_b_known) ||
        !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK ||
        input->accumulator_b_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status adds_smem_complete_for(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    const adds_smem_fields *fields,
    uint16_t read_value_word, ti_c54x_op_effect *next_effect)
{
    ti_c54x_u40_t addend;
    ti_c54x_u40_t base;
    ti_c54x_u40_add_result add_result;
    ti_c54x_op_effect staged;
    ti_c54x_u40_t value;
    bool carry;
    ti_c54x_signed32_overflow_direction overflow;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || fields == NULL ||
        next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    (void)instruction;
    status = adds_smem_validate_inputs(input, fields->select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* ADDS treats Smem as an unsigned 16-bit number regardless of SXM:
     * always zero-extended, never sign-extended. */
    addend = ti_c54x_u40_from_unsigned(read_value_word);
    base = fields->select == UINT8_C(0) ? input->accumulator_a_raw40
                                       : input->accumulator_b_raw40;
    if (ti_c54x_u40_add_checked(base, addend, &add_result) !=
        TI_C54X_ARITHMETIC40_ADDSUB_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    value = add_result.value;
    carry = add_result.low32_carry;
    overflow = add_result.signed32_overflow;
    if (input->status_ovm != UINT8_C(0)) {
        if (overflow == TI_C54X_SIGNED32_OVERFLOW_POSITIVE) {
            value = UINT64_C(0x007fffffff);
        } else if (overflow == TI_C54X_SIGNED32_OVERFLOW_NEGATIVE) {
            value = UINT64_C(0xff80000000);
        }
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields->select;
    staged.payload.accumulator_write.value_raw40 = value;
    staged.payload.accumulator_write.status_delta.c_action =
        carry ? TI_C54X_OP_STATUS_BIT_WRITE_1
              : TI_C54X_OP_STATUS_BIT_WRITE_0;
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.destination_overflow_action =
        overflow == TI_C54X_SIGNED32_OVERFLOW_NONE
            ? TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE
            : TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1;
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status adds_smem_stage_read(
    const adds_smem_fields *fields,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.complete = complete;
    staged.payload.memory.load_t_with_read = false;
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

static ti_c54x_op_status adds_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    adds_smem_fields fields;
    ti_c54x_op_status status;

    status = adds_smem_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return adds_smem_complete_for(instruction, input, &fields,
                                  read_value_word, next_effect);
}

ti_c54x_op_status ti_c54x_op_adds_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    adds_smem_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = adds_smem_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = adds_smem_validate_inputs(input, fields.select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return adds_smem_stage_read(&fields, adds_smem_complete, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_adds_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    adds_smem_fields fields;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields.select = (uint8_t)operands->values[0].raw_value;
    fields.indirect = (uint8_t)operands->values[1].raw_value;
    fields.addr7 = (uint8_t)operands->values[2].raw_value;
    if (operands->values[0].width_bits != UINT8_C(1) ||
        fields.select > UINT8_C(1) ||
        operands->values[1].width_bits != UINT8_C(1) ||
        fields.indirect > UINT8_C(1) ||
        operands->values[2].width_bits != UINT8_C(7) ||
        fields.addr7 > UINT8_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = adds_smem_validate_inputs(input, fields.select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return adds_smem_stage_read(&fields, adds_smem_complete, effect);
}
