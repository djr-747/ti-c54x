/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the exact
 * two-word DLD *(abs),dst LONG32 load (TI SPRU172C section
 * DLD with absolute addressing: first word 0x56F8 with D
 * selecting the destination accumulator and I=1 with the
 * 0x78 absolute selector, 16-bit absolute DATA address,
 * execution loads the long word at abs/abs+1 into
 * dst(31:0) with SXM widening, no status bits affected).
 * The checked leaf validates the D-masked exact first
 * word locally, requires the DLD_SMEM_ABSOLUTE opcode
 * id, validates the
 * ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 layout id and
 * 2-word count, gates SXM knownness, and extracts
 * (destination, indirect, selector, absolute) through the
 * layout set; the trusted leaf takes 4 operands with exact
 * widths (1,1,7,16). Both paths share one values core that
 * stages a fire-and-forget absolute LONG32 READ effect
 * (staged-zero halves, READ access) with the long32
 * accumulator-widening completion. Only the evidenced
 * absolute selector composes; every other first word or
 * cookie rejects. Borrowed instruction, operand, input,
 * and effect objects are used for one call only; no pointer
 * is retained and failed calls leave the caller effect
 * unchanged.
 * Does not own: opcode inventory, catalog rows, dispatch bindings, operand
 * layouts, address resolution, buses, CPU/state mutation, sequencing,
 * timing, observers, diagnostics, allocation, logging, or environment.
 */

#include "op_dld_smem_absolute.h"

#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define DLD_SMEM_ABSOLUTE_D_MASK UINT16_C(0xFEFF)
#define DLD_SMEM_ABSOLUTE_D_MASKED_WORD UINT16_C(0x56F8)
#define DLD_SMEM_ABSOLUTE_SELECTOR UINT8_C(0x78)

typedef struct dld_smem_absolute_fields {
    uint8_t destination;
    uint8_t indirect;
    uint8_t selector;
    uint16_t absolute;
} dld_smem_absolute_fields;

static ti_c54x_op_status dld_smem_absolute_values(
    const dld_smem_absolute_fields *fields,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (fields == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    /* Only the evidenced absolute selector composes: I=1 with
     * 0x78 selects absolute, every other cookie (direct I=0,
     * long-offset indirect) rejects. */
    if (fields->indirect != UINT8_C(1) ||
        fields->selector != DLD_SMEM_ABSOLUTE_SELECTOR) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (fields->destination > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    /* SXM unknownness gates the widening before any bus
     * traffic (MOD13 DLD precedent: DLD consumes SXM
     * only). */
    if (!input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_sxm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.first_value_msw = UINT16_C(0);
    staged.payload.memory.second_value_lsw = UINT16_C(0);
    staged.payload.memory.complete_long32 =
        ti_c54x_dld_smem_absolute_complete;
    staged.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_LOAD_WORD16;
    staged.payload.memory.address_spec.long32_absolute.absolute =
        fields->absolute;
    staged.payload.memory.address_spec.long32_absolute.reserved_half =
        UINT16_C(0);
    staged.payload.memory.address_spec.long32_absolute.reserved_word0 =
        UINT32_C(0);
    staged.payload.memory.address_spec.long32_absolute.reserved_word1 =
        UINT32_C(0);
    staged.payload.memory.address_spec.long32_absolute.reserved_word2 =
        UINT32_C(0);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status dld_smem_absolute_extract(
    const ti_c54x_decoded_instruction *instruction,
    dld_smem_absolute_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value selector_field = {0U, 0U};
    ti_c54x_operand_field_value absolute_field = {0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_DLD_SMEM_ABSOLUTE ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   DLD_SMEM_ABSOLUTE_D_MASK) !=
            DLD_SMEM_ABSOLUTE_D_MASKED_WORD) {
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
                &layout_view, &words, UINT8_C(0), &destination_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            destination_field.width_bits != UINT8_C(1) ||
            destination_field.raw_value > UINT64_C(1) ||
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

    fields->destination = (uint8_t)destination_field.raw_value;
    fields->indirect = (uint8_t)indirect_field.raw_value;
    fields->selector = (uint8_t)selector_field.raw_value;
    fields->absolute = (uint16_t)absolute_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_dld_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    dld_smem_absolute_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = dld_smem_absolute_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return dld_smem_absolute_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_dld_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    dld_smem_absolute_fields fields;

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

    fields.destination = (uint8_t)operands->values[0].raw_value;
    fields.indirect = (uint8_t)operands->values[1].raw_value;
    fields.selector = (uint8_t)operands->values[2].raw_value;
    fields.absolute = (uint16_t)operands->values[3].raw_value;
    return dld_smem_absolute_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_dld_smem_absolute_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t first_msw,
    uint16_t second_lsw, ti_c54x_op_effect *next_effect)
{
    dld_smem_absolute_fields fields;
    ti_c54x_op_status status;
    ti_c54x_u40_t value_raw40;
    ti_c54x_op_effect staged;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = dld_smem_absolute_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* The completion re-proves the shape and SXM knownness
     * against the frozen copies before publishing. */
    if (!input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_sxm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    value_raw40 = ti_c54x_u40_from_unsigned(
        ((uint64_t)first_msw << 16U) | (uint64_t)second_lsw);
    if (input->status_sxm != UINT8_C(0) &&
        (first_msw & UINT16_C(0x8000)) != UINT16_C(0)) {
        value_raw40 = ti_c54x_u40_from_unsigned(
            value_raw40 | UINT64_C(0xFF00000000));
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = fields.destination,
            .value_raw40 = value_raw40,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE}}};
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}
