/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the exact
 * two-word DST src,*(abs) WORD32 store (TI SPRU172C section
 * DST with absolute addressing: first word 0x4EF8 with S
 * selecting the accumulator and I=1 with the 0x78 absolute
 * selector, 16-bit absolute DATA address, execution
 * src(31:0) -> Lmem with MSW at the absolute address and
 * LSW above it, no status bits affected). The checked leaf
 * validates the exact first word locally, requires the
 * DST_SMEM_ABSOLUTE opcode id, validates the
 * ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 layout id and
 * 2-word count, and extracts (source, indirect, selector,
 * absolute) through the layout set; the trusted leaf takes
 * 4 operands with exact widths (1,1,7,16). Both paths share
 * one values core that gates accumulator plus SXM/SST
 * knownness, applies the MOD13 saturation (SST-gated
 * SXM-signed/unsigned 32-bit clamp, low 32 bits), and stages
 * a fire-and-forget absolute LONG32 store effect (WRITE
 * access, staged MSW/LSW halves) with no MMR sidecar, no T
 * traffic, and no read-value completion. Only the evidenced
 * absolute selector composes; every other first word or
 * cookie rejects. Borrowed instruction, operand, input, and
 * effect objects are used for one call only; no pointer is
 * retained and failed calls leave the caller effect
 * unchanged.
 * Does not own: opcode inventory, catalog rows, dispatch bindings, operand
 * layouts, address resolution, buses, CPU/state mutation, sequencing,
 * timing, observers, diagnostics, allocation, logging, or environment.
 */

#include "op_dst_smem_absolute.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define DST_SMEM_ABSOLUTE_S_MASK UINT16_C(0xFEFF)
#define DST_SMEM_ABSOLUTE_S_MASKED_WORD UINT16_C(0x4EF8)
#define DST_SMEM_ABSOLUTE_SELECTOR UINT8_C(0x78)

typedef struct dst_smem_absolute_fields {
    uint8_t source;
    uint8_t indirect;
    uint8_t selector;
    uint16_t absolute;
} dst_smem_absolute_fields;

static ti_c54x_op_status dst_smem_absolute_values(
    const dst_smem_absolute_fields *fields,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t raw;
    ti_c54x_u40_t stored;
    bool known;
    ti_c54x_op_effect staged = {0};

    if (fields == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    /* Only the evidenced absolute selector composes: I=1 with
     * 0x78 selects absolute, every other cookie (direct I=0,
     * long-offset indirect) rejects. */
    if (fields->indirect != UINT8_C(1) ||
        fields->selector != DST_SMEM_ABSOLUTE_SELECTOR) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (fields->source > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    /* SXM/SST unknownness gates before any bus traffic (MOD13
     * DST precedent); accumulator unknownness rejects
     * transactionally downstream-neutral. */
    if (!input->status_sxm_known || !input->status_sst_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_sxm > UINT8_C(1) ||
        input->status_sst > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (fields->source == UINT8_C(0)) {
        raw = input->accumulator_a_raw40;
        known = input->accumulator_a_known;
    } else {
        raw = input->accumulator_b_raw40;
        known = input->accumulator_b_known;
    }
    if (!known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (raw > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    stored = raw;
    if (input->status_sst != UINT8_C(0)) {
        if (input->status_sxm == UINT8_C(0)) {
            if (raw > UINT64_C(0xFFFFFFFF)) {
                stored = UINT64_C(0xFFFFFFFF);
            }
        } else {
            const int64_t signed_raw = ti_c54x_u40_sign_extend(raw);
            if (signed_raw > INT64_C(0x7FFFFFFF)) {
                stored = UINT64_C(0x7FFFFFFF);
            } else if (signed_raw < -INT64_C(0x80000000)) {
                stored = UINT64_C(0x80000000);
            }
        }
    }
    stored &= UINT64_C(0xFFFFFFFF);

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    staged.payload.memory.first_value_msw =
        (uint16_t)(stored >> 16U);
    staged.payload.memory.second_value_lsw = (uint16_t)stored;
    staged.payload.memory.complete_long32 = NULL;
    staged.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_LONG32_ABSOLUTE_STORE_WORD16;
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

static ti_c54x_op_status dst_smem_absolute_extract(
    const ti_c54x_decoded_instruction *instruction,
    dst_smem_absolute_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value selector_field = {0U, 0U};
    ti_c54x_operand_field_value absolute_field = {0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_DST_SMEM_ABSOLUTE ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   DST_SMEM_ABSOLUTE_S_MASK) !=
            DST_SMEM_ABSOLUTE_S_MASKED_WORD) {
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
                &layout_view, &words, UINT8_C(0), &source_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            source_field.width_bits != UINT8_C(1) ||
            source_field.raw_value > UINT64_C(1) ||
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

    fields->source = (uint8_t)source_field.raw_value;
    fields->indirect = (uint8_t)indirect_field.raw_value;
    fields->selector = (uint8_t)selector_field.raw_value;
    fields->absolute = (uint16_t)absolute_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_dst_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    dst_smem_absolute_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = dst_smem_absolute_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return dst_smem_absolute_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_dst_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    dst_smem_absolute_fields fields;

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

    fields.source = (uint8_t)operands->values[0].raw_value;
    fields.indirect = (uint8_t)operands->values[1].raw_value;
    fields.selector = (uint8_t)operands->values[2].raw_value;
    fields.absolute = (uint16_t)operands->values[3].raw_value;
    return dst_smem_absolute_values(&fields, input, effect);
}
