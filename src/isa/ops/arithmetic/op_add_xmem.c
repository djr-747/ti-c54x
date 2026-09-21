/* Module contract
 * Owns: pure checked/trusted ADD Xmem syntax-5 semantics (src +=
 * SXM-widened Xmem << SHFT, dst is always src) over the shared
 * one-word accumulator/dual-Xmod/Xar/shift4 layout with the
 * 0x9000 fixed prefix, staging an X-only dual-indirect WORD16
 * read through the memory effect, with OVM saturation plus sticky
 * OVsrc. Xmem is sign-extended iff SXM=1 else zero-extended,
 * then shifted left by the unsigned 4-bit SHFT. C is written
 * from the low-32 carry; TC is preserved.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_add_xmem.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "foundation/arithmetic40_addsub.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/memory/memory_effect.h"

/* Fixed prefix for ADD syn5: word0 = 0x9000 | S<<8 | XXXX<<4 | SHFT. */
#define ADD_XMEM_FIXED_MASK UINT16_C(0xFE00)
#define ADD_XMEM_FIXED_VALUE UINT16_C(0x9000)

typedef struct add_xmem_fields {
    uint8_t select;
    uint8_t x_mode;
    uint8_t x_ar;
    uint8_t shift;
} add_xmem_fields;

static ti_c54x_op_status add_xmem_extract(
    const ti_c54x_decoded_instruction *instruction,
    add_xmem_fields *fields)
{
    static const uint8_t want_width[4] = {1U, 2U, 2U, 4U};
    static const uint64_t want_max[4] = {1U, 3U, 3U, 15U};
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    uint8_t index;
    uint8_t got[4] = {0U, 0U, 0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4 ||
        instruction->bundle.program_word_count != UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if ((uint16_t)(instruction->bundle.program_words[0] &
                   ADD_XMEM_FIXED_MASK) != ADD_XMEM_FIXED_VALUE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4,
            UINT8_C(1), &layout_view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < UINT8_C(4); ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != want_width[index] ||
            field.raw_value > want_max[index]) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        got[index] = (uint8_t)field.raw_value;
    }
    fields->select = got[0];
    fields->x_mode = got[1];
    fields->x_ar = (uint8_t)(got[2] + UINT8_C(2));
    fields->shift = got[3];
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status add_xmem_validate_inputs(
    const ti_c54x_op_input_snapshot *input, uint8_t select)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if ((select == UINT8_C(0) ? !input->accumulator_a_known
                             : !input->accumulator_b_known) ||
        !input->status_sxm_known || !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK ||
        input->accumulator_b_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (input->status_sxm > UINT8_C(1) ||
        input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status add_xmem_complete_for(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    const add_xmem_fields *fields,
    uint16_t read_value_word, ti_c54x_op_effect *next_effect)
{
    ti_c54x_u40_t addend;
    ti_c54x_u40_t base;
    ti_c54x_u40_add_result add_result;
    ti_c54x_op_effect staged = {0};
    ti_c54x_u40_t value;
    bool carry;
    ti_c54x_signed32_overflow_direction overflow;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || fields == NULL ||
        next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    (void)instruction;
    status = add_xmem_validate_inputs(input, fields->select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* SXM-widen the Xmem word, then shift left: low bits clear and
     * the widened high bits above the 16-bit field carry the SXM
     * sign extension (or zeros when SXM=0). */
    addend = ti_c54x_u40_from_unsigned(read_value_word);
    if (input->status_sxm != UINT8_C(0) &&
        (read_value_word & UINT16_C(0x8000)) != UINT16_C(0)) {
        addend = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                 (uint64_t)read_value_word;
    }
    addend = ti_c54x_u40_logical_left_shift(addend, fields->shift);
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
    /* C is written from the bit-32 carry like the established ADD
     * leaves; TC is preserved. Dst is always src, so the sticky
     * overflow lands on OVsrc. */
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

static ti_c54x_op_status add_xmem_stage_read(
    const add_xmem_fields *fields,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    /* Y self-aliases X: the X-only resolver reads and commits the X
     * side alone through the single-read child, as in the LD Xmem
     * shift leaf. */
    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.complete = complete;
    staged.payload.memory.load_t_with_read = false;
    staged.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY;
    staged.payload.memory.address_spec.dual.x_mode = fields->x_mode;
    staged.payload.memory.address_spec.dual.x_ar = fields->x_ar;
    staged.payload.memory.address_spec.dual.y_mode = fields->x_mode;
    staged.payload.memory.address_spec.dual.y_ar = fields->x_ar;
    staged.payload.memory.address_spec.dual.reserved_word0 = UINT32_C(0);
    staged.payload.memory.address_spec.dual.reserved_word1 = UINT32_C(0);
    staged.payload.memory.address_spec.dual.reserved_word2 = UINT32_C(0);
    staged.payload.memory.address_spec.dual.reserved_word3 = UINT32_C(0);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status add_xmem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    add_xmem_fields fields;
    ti_c54x_op_status status;

    status = add_xmem_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return add_xmem_complete_for(instruction, input, &fields,
                                 read_value_word, next_effect);
}

ti_c54x_op_status ti_c54x_op_add_xmem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    add_xmem_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = add_xmem_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = add_xmem_validate_inputs(input, fields.select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return add_xmem_stage_read(&fields, add_xmem_complete, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_add_xmem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    static const uint8_t widths[4] = {1U, 2U, 2U, 4U};
    static const uint64_t maxima[4] = {1U, 3U, 3U, 15U};
    add_xmem_fields fields;
    uint64_t raw[4] = {0U, 0U, 0U, 0U};
    uint8_t index;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(4)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    for (index = UINT8_C(0); index < UINT8_C(4); ++index) {
        if (operands->values[index].width_bits != widths[index] ||
            operands->values[index].raw_value > maxima[index]) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        raw[index] = operands->values[index].raw_value;
    }
    fields.select = (uint8_t)raw[0];
    fields.x_mode = (uint8_t)raw[1];
    fields.x_ar = (uint8_t)(raw[2] + UINT64_C(2));
    fields.shift = (uint8_t)raw[3];
    status = add_xmem_validate_inputs(input, fields.select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return add_xmem_stage_read(&fields, add_xmem_complete, effect);
}
