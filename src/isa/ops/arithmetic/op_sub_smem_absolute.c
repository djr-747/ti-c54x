/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the exact
 * two-word SUB *(abs),src WORD16 load (TI SPRU172C
 * section SUB syntax 1 with absolute addressing:
 * first word 0x08F8 with S selecting the source
 * accumulator and I=1 with the 0x78 absolute selector,
 * 16-bit absolute DATA address, execution computes
 * src minus the SXM-widened word at abs with OVM
 * saturation on signed-32 overflow, C written from
 * the bit-32 borrow with TC preserved and OV set on
 * overflow). The checked leaf validates the S-masked
 * exact first word locally, requires the
 * SUB_SMEM_ABSOLUTE opcode id, validates the
 * ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 layout id and
 * 2-word count, gates source-accumulator, SXM, and
 * OVM knownness, and extracts (select, indirect,
 * selector, absolute) through the layout set; the
 * trusted leaf takes 4 operands with exact widths
 * (1,1,7,16). Both paths share one values core that
 * stages a fire-and-forget absolute WORD16 READ effect
 * (staged-zero word, READ access) with the SUB
 * syntax-1 completion. Only the evidenced absolute
 * selector composes; every other first word or cookie
 * rejects. Borrowed instruction, operand, input, and
 * effect objects are used for one call only; no pointer
 * is retained and failed calls leave the caller effect
 * unchanged.
 * Does not own: opcode inventory, catalog rows, dispatch bindings, operand
 * layouts, address resolution, buses, CPU/state mutation, sequencing,
 * timing, observers, diagnostics, allocation, logging, or environment.
 */

#include "op_sub_smem_absolute.h"

#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "foundation/arithmetic40_addsub.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define SUB_SMEM_ABSOLUTE_S_MASK UINT16_C(0xFEFF)
#define SUB_SMEM_ABSOLUTE_S_MASKED_WORD UINT16_C(0x08F8)
#define SUB_SMEM_ABSOLUTE_SELECTOR UINT8_C(0x78)

typedef struct sub_smem_absolute_fields {
    uint8_t select;
    uint8_t indirect;
    uint8_t selector;
    uint16_t absolute;
} sub_smem_absolute_fields;

static ti_c54x_op_status sub_smem_absolute_validate_inputs(
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

static ti_c54x_op_status sub_smem_absolute_values(
    const sub_smem_absolute_fields *fields,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_status status;

    if (fields == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    /* Only the evidenced absolute selector composes: I=1 with
     * 0x78 selects absolute, every other cookie (direct I=0,
     * long-offset indirect) rejects. */
    if (fields->indirect != UINT8_C(1) ||
        fields->selector != SUB_SMEM_ABSOLUTE_SELECTOR) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (fields->select > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    /* Source, SXM, and OVM unknownness gate the read before
     * any bus traffic (SUB_SMEM precedent: the family
     * consumes ACCUMULATORS_STATUS_SXM_OVM). */
    status =
        sub_smem_absolute_validate_inputs(input, fields->select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.complete = ti_c54x_sub_smem_absolute_complete;
    staged.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16;
    staged.payload.memory.address_spec.absolute_data.address =
        fields->absolute;
    staged.payload.memory.address_spec.absolute_data.mmr_sidecar_kind =
        (uint8_t)TI_C54X_OP_MEMORY_MMR_SIDECAR_NONE;
    staged.payload.memory.address_spec.absolute_data.mmr_sidecar_number =
        UINT8_C(0);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status sub_smem_absolute_extract(
    const ti_c54x_decoded_instruction *instruction,
    sub_smem_absolute_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value select_field = {0U, 0U};
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value selector_field = {0U, 0U};
    ti_c54x_operand_field_value absolute_field = {0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_SUB_SMEM_ABSOLUTE ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   SUB_SMEM_ABSOLUTE_S_MASK) !=
            SUB_SMEM_ABSOLUTE_S_MASKED_WORD) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16,
            UINT8_C(2), &layout_view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};

        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &select_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            select_field.width_bits != UINT8_C(1) ||
            select_field.raw_value > UINT64_C(1) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &indirect_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            indirect_field.width_bits != UINT8_C(1) ||
            indirect_field.raw_value > UINT64_C(1) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &selector_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            selector_field.width_bits != UINT8_C(7) ||
            selector_field.raw_value > UINT64_C(0x7F) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(3), &absolute_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            absolute_field.width_bits != UINT8_C(16) ||
            absolute_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    fields->select = (uint8_t)select_field.raw_value;
    fields->indirect = (uint8_t)indirect_field.raw_value;
    fields->selector = (uint8_t)selector_field.raw_value;
    fields->absolute = (uint16_t)absolute_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_sub_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    sub_smem_absolute_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sub_smem_absolute_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return sub_smem_absolute_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_sub_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    sub_smem_absolute_fields fields;

    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(4) ||
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

    fields.select = (uint8_t)operands->values[0].raw_value;
    fields.indirect = (uint8_t)operands->values[1].raw_value;
    fields.selector = (uint8_t)operands->values[2].raw_value;
    fields.absolute = (uint16_t)operands->values[3].raw_value;
    return sub_smem_absolute_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_sub_smem_absolute_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    sub_smem_absolute_fields fields;
    ti_c54x_op_status status;
    ti_c54x_u40_t subtrahend;
    ti_c54x_u40_t base;
    ti_c54x_u40_sub_result sub_result;
    ti_c54x_u40_t value;
    bool no_borrow;
    ti_c54x_signed32_overflow_direction overflow;
    ti_c54x_op_effect staged;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = sub_smem_absolute_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* The completion re-proves the shape and input
     * knownness against the frozen copies before publishing. */
    status = sub_smem_absolute_validate_inputs(input, fields.select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* SXM-widened subtrahend (SUB_SMEM precedent). */
    subtrahend = ti_c54x_u40_from_unsigned(read_value_word);
    if (input->status_sxm != UINT8_C(0) &&
        (read_value_word & UINT16_C(0x8000)) != UINT16_C(0)) {
        subtrahend = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                     (uint64_t)read_value_word;
    }
    base = fields.select == UINT8_C(0) ? input->accumulator_a_raw40
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
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = fields.select,
            .value_raw40 = value,
            /* C set means no borrow (SUB_SMEM precedent);
             * TC preserves; OV writes on overflow. */
            .status_delta = {
                .c_action = no_borrow
                                ? TI_C54X_OP_STATUS_BIT_WRITE_1
                                : TI_C54X_OP_STATUS_BIT_WRITE_0,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .destination_overflow_action =
                    overflow == TI_C54X_SIGNED32_OVERFLOW_NONE
                        ? TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE
                        : TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1}}};
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}
