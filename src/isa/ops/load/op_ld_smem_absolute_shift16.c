/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the exact
 * two-word LD *(abs),16,dst shift-16 load (TI SPRU172C
 * section LD with absolute addressing and shift 16: first
 * word 0x44F8 with D selecting the destination accumulator
 * and I=1 with the 0x78 absolute selector, 16-bit absolute
 * DATA address, execution loads the word at abs into
 * dst(31:16) with SXM widening then logical-left-shift
 * 16, no status bits affected). The checked leaf validates
 * the D-masked exact first word locally, requires the
 * LD_SMEM_ABSOLUTE_SHIFT16 opcode id, validates the
 * ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 layout id and
 * 2-word count, gates SXM knownness, and extracts
 * (destination, indirect, selector, absolute) through the
 * layout set; the trusted leaf takes 4 operands with exact
 * widths (1,1,7,16). Both paths share one values core that
 * stages a fire-and-forget absolute WORD16 READ effect
 * (staged-zero word, READ access) with the shift16
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

#include "op_ld_smem_absolute_shift16.h"

#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define LD_SMEM_ABSOLUTE_SHIFT16_D_MASK UINT16_C(0xFEFF)
#define LD_SMEM_ABSOLUTE_SHIFT16_D_MASKED_WORD UINT16_C(0x44F8)
#define LD_SMEM_ABSOLUTE_SHIFT16_SELECTOR UINT8_C(0x78)

typedef struct ld_smem_absolute_shift16_fields {
    uint8_t destination;
    uint8_t indirect;
    uint8_t selector;
    uint16_t absolute;
} ld_smem_absolute_shift16_fields;

static ti_c54x_op_status ld_smem_absolute_shift16_values(
    const ld_smem_absolute_shift16_fields *fields,
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
        fields->selector != LD_SMEM_ABSOLUTE_SHIFT16_SELECTOR) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (fields->destination > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    /* SXM unknownness gates the widening before any bus
     * traffic (MOD shift16 precedent: the family consumes
     * SXM only). */
    if (!input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_sxm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.complete =
        ti_c54x_ld_smem_absolute_shift16_complete;
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

static ti_c54x_op_status ld_smem_absolute_shift16_extract(
    const ti_c54x_decoded_instruction *instruction,
    ld_smem_absolute_shift16_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value selector_field = {0U, 0U};
    ti_c54x_operand_field_value absolute_field = {0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_LD_SMEM_ABSOLUTE_SHIFT16 ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   LD_SMEM_ABSOLUTE_SHIFT16_D_MASK) !=
            LD_SMEM_ABSOLUTE_SHIFT16_D_MASKED_WORD) {
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

ti_c54x_op_status ti_c54x_op_ld_smem_absolute_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ld_smem_absolute_shift16_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_smem_absolute_shift16_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ld_smem_absolute_shift16_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_ld_smem_absolute_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ld_smem_absolute_shift16_fields fields;

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
    return ld_smem_absolute_shift16_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_ld_smem_absolute_shift16_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ld_smem_absolute_shift16_fields fields;
    ti_c54x_op_status status;
    ti_c54x_u40_t value_raw40;
    ti_c54x_op_effect staged;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_smem_absolute_shift16_extract(instruction, &fields);
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
    /* SXM-widen then logical-left-shift 16 (MOD shift16
     * precedent): widening first preserves the sign through
     * the shift, so a negative word sets the guard bits. */
    value_raw40 = ti_c54x_u40_from_unsigned(read_value_word);
    if (input->status_sxm != UINT8_C(0) &&
        (read_value_word & UINT16_C(0x8000)) != UINT16_C(0)) {
        value_raw40 = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                      (uint64_t)read_value_word;
    }
    value_raw40 =
        ti_c54x_u40_logical_left_shift(value_raw40, UINT8_C(16));
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
