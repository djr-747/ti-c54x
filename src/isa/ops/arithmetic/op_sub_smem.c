/* Module contract
 * Owns: pure checked/trusted SUB Smem semantics for syntaxes 1-3
 * (unshifted src -= Smem, T-shifted src -= Smem<<TS, and
 * dst = src - (Smem << 16)) over direct page-0 and
 * standard-indirect MOD0--MOD3 reads through the memory effect.
 * Exotic indirect modes (mode4 above MOD3) defer as
 * INVALID_OPERAND; T-shift counts outside the established
 * 0x00--0x1F/0x30--0x3F bands reject as INVALID_OPERAND.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_sub_smem.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "foundation/arithmetic40_addsub.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/memory/memory_effect.h"

typedef enum sub_smem_syntax {
    SUB_SMEM_SYNTAX_UNSHIFTED = 0,
    SUB_SMEM_SYNTAX_TS = 1,
    SUB_SMEM_SYNTAX_SH16 = 2
} sub_smem_syntax;

typedef struct sub_smem_fields {
    uint8_t select;
    uint8_t destination;
    uint8_t indirect;
    uint8_t addr7;
} sub_smem_fields;

static ti_c54x_op_status sub_smem_extract(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_operand_layout_id expected_layout,
    uint8_t operand_count,
    sub_smem_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    uint8_t index;
    uint8_t got[4] = {0U, 0U, 0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->operand_layout != expected_layout ||
        instruction->bundle.program_word_count != UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            expected_layout, UINT8_C(1),
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < operand_count; ++index) {
        const uint8_t want_width =
            index == (uint8_t)(operand_count - UINT8_C(1)) ? UINT8_C(7)
                                                           : UINT8_C(1);
        const uint64_t want_max =
            index == (uint8_t)(operand_count - UINT8_C(1))
                ? UINT64_C(0x7F)
                : UINT64_C(1);
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != want_width ||
            field.raw_value > want_max) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        got[index] = (uint8_t)field.raw_value;
    }
    if (operand_count == UINT8_C(3)) {
        fields->select = got[0];
        fields->destination = got[0];
        fields->indirect = got[1];
        fields->addr7 = got[2];
    } else {
        fields->select = got[0];
        fields->destination = got[1];
        fields->indirect = got[2];
        fields->addr7 = got[3];
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sub_smem_validate_inputs(
    const ti_c54x_op_input_snapshot *input, uint8_t select,
    uint8_t destination, bool need_t)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if ((select == UINT8_C(0) ? !input->accumulator_a_known
                             : !input->accumulator_b_known) ||
        (destination != select &&
         (destination == UINT8_C(0) ? !input->accumulator_a_known
                                   : !input->accumulator_b_known)) ||
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
    if (need_t) {
        const uint8_t shift_bits =
            (uint8_t)(input->t_register_raw16 & UINT16_C(0x3F));
        if (!input->t_register_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
        if (shift_bits >= UINT8_C(0x20) && shift_bits <= UINT8_C(0x2F)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_u40_t sub_smem_shift_subtrahend(ti_c54x_u40_t widened,
    sub_smem_syntax syntax,
    const ti_c54x_op_input_snapshot *input)
{
    if (syntax == SUB_SMEM_SYNTAX_SH16) {
        return ti_c54x_u40_logical_left_shift(widened, UINT64_C(16));
    }
    if (syntax == SUB_SMEM_SYNTAX_TS) {
        const uint8_t shift_bits =
            (uint8_t)(input->t_register_raw16 & UINT16_C(0x3F));
        if (shift_bits <= UINT8_C(31)) {
            return ti_c54x_u40_logical_left_shift(widened, shift_bits);
        }
        return ti_c54x_u40_arithmetic_right_shift(
            widened, (uint64_t)(UINT8_C(64) - shift_bits));
    }
    return widened;
}

static ti_c54x_op_status sub_smem_complete_for(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    const sub_smem_fields *fields, sub_smem_syntax syntax,
    uint16_t read_value_word, ti_c54x_op_effect *next_effect)
{
    ti_c54x_u40_t subtrahend;
    ti_c54x_u40_t base;
    ti_c54x_u40_sub_result sub_result;
    ti_c54x_op_effect staged;
    ti_c54x_u40_t value;
    bool no_borrow;
    ti_c54x_signed32_overflow_direction overflow;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || fields == NULL ||
        next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    (void)instruction;
    status = sub_smem_validate_inputs(input, fields->select,
                                      fields->destination,
                                      syntax == SUB_SMEM_SYNTAX_TS);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    subtrahend = ti_c54x_u40_from_unsigned(read_value_word);
    if (input->status_sxm != UINT8_C(0) &&
        (read_value_word & UINT16_C(0x8000)) != UINT16_C(0)) {
        subtrahend = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                 (uint64_t)read_value_word;
    }
    subtrahend = sub_smem_shift_subtrahend(subtrahend, syntax, input);
    base = fields->select == UINT8_C(0) ? input->accumulator_a_raw40
                                       : input->accumulator_b_raw40;
    if (ti_c54x_u40_sub_checked(base, subtrahend, &sub_result) !=
        TI_C54X_ARITHMETIC40_ADDSUB_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    value = sub_result.value;
    no_borrow = sub_result.low32_no_borrow;
    overflow = sub_result.signed32_overflow;
    if (input->status_ovm != UINT8_C(0)) {
        if (overflow == TI_C54X_SIGNED32_OVERFLOW_POSITIVE) {
            value = UINT64_C(0x007fffffff);
        } else if (overflow == TI_C54X_SIGNED32_OVERFLOW_NEGATIVE) {
            value = UINT64_C(0xff80000000);
        }
    }
    staged.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
    staged.payload.accumulator_write.destination = fields->destination;
    staged.payload.accumulator_write.value_raw40 = value;
    /* Syntaxes 1-2 write C from the bit-32 borrow like the established
     * SUB leaves (C set means no borrow); syntax 3 sets C on no-borrow
     * and otherwise preserves it, mirroring the explicit TI rule for
     * the fixed <<16 form on the ADD side. */
    staged.payload.accumulator_write.status_delta.c_action =
        syntax == SUB_SMEM_SYNTAX_SH16
            ? (no_borrow ? TI_C54X_OP_STATUS_BIT_WRITE_1
                         : TI_C54X_OP_STATUS_BIT_PRESERVE)
            : (no_borrow ? TI_C54X_OP_STATUS_BIT_WRITE_1
                         : TI_C54X_OP_STATUS_BIT_WRITE_0);
    staged.payload.accumulator_write.status_delta.tc_action =
        TI_C54X_OP_STATUS_BIT_PRESERVE;
    staged.payload.accumulator_write.status_delta.destination_overflow_action =
        overflow == TI_C54X_SIGNED32_OVERFLOW_NONE
            ? TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE
            : TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1;
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sub_smem_stage_read(
    const sub_smem_fields *fields,
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

static ti_c54x_op_status sub_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    sub_smem_fields fields;
    ti_c54x_op_status status;

    status = sub_smem_extract(instruction,
                              TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7,
                              UINT8_C(3), &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sub_smem_complete_for(instruction, input, &fields,
                                 SUB_SMEM_SYNTAX_UNSHIFTED,
                                 read_value_word, next_effect);
}

static ti_c54x_op_status sub_smem_ts_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    sub_smem_fields fields;
    ti_c54x_op_status status;

    status = sub_smem_extract(instruction,
                              TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7,
                              UINT8_C(3), &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sub_smem_complete_for(instruction, input, &fields,
                                 SUB_SMEM_SYNTAX_TS,
                                 read_value_word, next_effect);
}

static ti_c54x_op_status sub_smem_sh16_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    sub_smem_fields fields;
    ti_c54x_op_status status;

    status = sub_smem_extract(
        instruction,
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7,
        UINT8_C(4), &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sub_smem_complete_for(instruction, input, &fields,
                                 SUB_SMEM_SYNTAX_SH16,
                                 read_value_word, next_effect);
}

static ti_c54x_op_status sub_smem_checked(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect,
    ti_c54x_operand_layout_id expected_layout, uint8_t operand_count,
    bool need_t,
    ti_c54x_op_memory_complete_fn complete)
{
    sub_smem_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sub_smem_extract(instruction, expected_layout,
                              operand_count, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = sub_smem_validate_inputs(input, fields.select,
                                      fields.destination, need_t);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sub_smem_stage_read(&fields, complete, effect);
}

ti_c54x_op_status ti_c54x_op_sub_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return sub_smem_checked(
        instruction, input, effect,
        TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7, UINT8_C(3), false,
        sub_smem_complete);
}

ti_c54x_op_status ti_c54x_op_sub_smem_ts(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return sub_smem_checked(
        instruction, input, effect,
        TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7, UINT8_C(3), true,
        sub_smem_ts_complete);
}

ti_c54x_op_status ti_c54x_op_sub_smem_sh16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    return sub_smem_checked(
        instruction, input, effect,
        TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7,
        UINT8_C(4), false, sub_smem_sh16_complete);
}

static ti_c54x_op_status sub_smem_trusted(
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect,
    uint8_t operand_count, sub_smem_syntax syntax,
    ti_c54x_op_memory_complete_fn complete)
{
    sub_smem_fields fields;
    ti_c54x_op_status status;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != operand_count) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields.select = (uint8_t)operands->values[0].raw_value;
    if (operands->values[0].width_bits != UINT8_C(1) || fields.select > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (operand_count == UINT8_C(4)) {
        fields.destination = (uint8_t)operands->values[1].raw_value;
        if (operands->values[1].width_bits != UINT8_C(1) ||
            fields.destination > UINT8_C(1)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        fields.indirect = (uint8_t)operands->values[2].raw_value;
        fields.addr7 = (uint8_t)operands->values[3].raw_value;
        if (operands->values[2].width_bits != UINT8_C(1) ||
            fields.indirect > UINT8_C(1) ||
            operands->values[3].width_bits != UINT8_C(7) ||
            fields.addr7 > UINT8_C(0x7F)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
    } else {
        fields.destination = fields.select;
        fields.indirect = (uint8_t)operands->values[1].raw_value;
        fields.addr7 = (uint8_t)operands->values[2].raw_value;
        if (operands->values[1].width_bits != UINT8_C(1) ||
            fields.indirect > UINT8_C(1) ||
            operands->values[2].width_bits != UINT8_C(7) ||
            fields.addr7 > UINT8_C(0x7F)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
    }
    status = sub_smem_validate_inputs(input, fields.select,
                                      fields.destination,
                                      syntax == SUB_SMEM_SYNTAX_TS);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sub_smem_stage_read(&fields, complete, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_sub_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;
    return sub_smem_trusted(operands, input, effect, UINT8_C(3),
                            SUB_SMEM_SYNTAX_UNSHIFTED, sub_smem_complete);
}

ti_c54x_op_status ti_c54x_trusted_op_sub_smem_ts(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;
    return sub_smem_trusted(operands, input, effect, UINT8_C(3),
                            SUB_SMEM_SYNTAX_TS, sub_smem_ts_complete);
}

ti_c54x_op_status ti_c54x_trusted_op_sub_smem_sh16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;
    return sub_smem_trusted(operands, input, effect, UINT8_C(4),
                            SUB_SMEM_SYNTAX_SH16, sub_smem_sh16_complete);
}
