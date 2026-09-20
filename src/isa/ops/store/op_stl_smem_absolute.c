/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the exact
 * two-word STL *(abs),src WORD16 store (TI SPRU172C
 * section STL with absolute addressing: first word
 * 0x80F8 with S selecting the source accumulator and
 * I=1 with the 0x78 absolute selector, 16-bit absolute
 * DATA address, execution stores src(15:0) to the word
 * at abs with no status bits affected). The checked
 * leaf validates the S-masked exact first word
 * locally, requires the STL_SMEM_ABSOLUTE opcode id,
 * validates the
 * ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 layout id and
 * 2-word count, gates source-accumulator knownness,
 * and extracts (source, indirect, selector, absolute)
 * through the layout set; the trusted leaf takes 4
 * operands with exact widths (1,1,7,16). Both paths
 * share one values core that stages a fire-and-forget
 * absolute WORD16 WRITE effect (staged src low word
 * rides verbatim, sidecar NONE, complete NULL). Only
 * the evidenced absolute selector composes; every
 * other first word or cookie rejects. Borrowed
 * instruction, operand, input, and effect objects are
 * used for one call only; no pointer is retained and
 * failed calls leave the caller effect unchanged.
 * Does not own: opcode inventory, catalog rows, dispatch bindings, operand
 * layouts, address resolution, buses, CPU/state mutation, sequencing,
 * timing, observers, diagnostics, allocation, logging, or environment.
 */

#include "op_stl_smem_absolute.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define STL_SMEM_ABSOLUTE_S_MASK UINT16_C(0xFEFF)
#define STL_SMEM_ABSOLUTE_S_MASKED_WORD UINT16_C(0x80F8)
#define STL_SMEM_ABSOLUTE_SELECTOR UINT8_C(0x78)

typedef struct stl_smem_absolute_fields {
    uint8_t source;
    uint8_t indirect;
    uint8_t selector;
    uint16_t absolute;
} stl_smem_absolute_fields;

static ti_c54x_op_status stl_smem_absolute_values(
    const stl_smem_absolute_fields *fields,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40;
    bool source_known;
    ti_c54x_op_effect staged = {0};

    if (fields == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    /* Only the evidenced absolute selector composes: I=1 with
     * 0x78 selects absolute, every other cookie (direct I=0,
     * long-offset indirect) rejects. */
    if (fields->indirect != UINT8_C(1) ||
        fields->selector != STL_SMEM_ABSOLUTE_SELECTOR) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (fields->source > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    /* Source unknownness gates the write before any bus
     * traffic (STL_SMEM precedent: the family consumes the
     * source accumulator only). */
    if (fields->source == UINT8_C(0)) {
        source_raw40 = input->accumulator_a_raw40;
        source_known = input->accumulator_a_known;
    } else {
        source_raw40 = input->accumulator_b_raw40;
        source_known = input->accumulator_b_known;
    }
    if (!source_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (source_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    /* Fire-and-forget absolute WRITE: the staged src low
     * word rides verbatim with sidecar NONE and no
     * completion (ST #lk precedent). */
    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    staged.payload.memory.value_word = (uint16_t)source_raw40;
    staged.payload.memory.complete = NULL;
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

static ti_c54x_op_status stl_smem_absolute_extract(
    const ti_c54x_decoded_instruction *instruction,
    stl_smem_absolute_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value selector_field = {0U, 0U};
    ti_c54x_operand_field_value absolute_field = {0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_STL_SMEM_ABSOLUTE ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16 ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   STL_SMEM_ABSOLUTE_S_MASK) !=
            STL_SMEM_ABSOLUTE_S_MASKED_WORD) {
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

ti_c54x_op_status ti_c54x_op_stl_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    stl_smem_absolute_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = stl_smem_absolute_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return stl_smem_absolute_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_stl_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    stl_smem_absolute_fields fields;

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
    return stl_smem_absolute_values(&fields, input, effect);
}
