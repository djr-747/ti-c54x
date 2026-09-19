/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the exact
 * three-word ADDM #lk,*(abs) WORD16 read-modify-write (TI
 * SPRU172C section ADDM with absolute addressing: first
 * word 0x6BF8 with I=1 and the 0x78 absolute selector,
 * 16-bit absolute DATA address in word 1, 16-bit immediate
 * in word 2, execution *(abs) += lk with OVM/SXM
 * saturation and sticky C/OVA commit, no other status
 * bits affected). The checked leaf validates the exact
 * first word locally, requires the ADDM_LK_SMEM_ABSOLUTE
 * opcode id, validates the INDIRECT_ADDR7_ABSOLUTE16_LK
 * layout id and 3-word count, gates OVM/SXM knownness,
 * and extracts (indirect, selector, absolute, lk) through
 * the layout set; the trusted leaf takes 4 operands with
 * exact widths (1,7,16,16). Both paths share one values
 * core that stages a fire-and-forget atomic absolute ADD
 * effect (READ access, staged immediate, RMW ADD, no
 * completion). Only the evidenced absolute selector
 * composes; every other first word or cookie rejects.
 * Borrowed instruction, operand, input, and effect objects
 * are used for one call only; no pointer is retained and
 * failed calls leave the caller effect unchanged.
 * Does not own: opcode inventory, catalog rows, dispatch bindings, operand
 * layouts, address resolution, buses, CPU/state mutation, sequencing,
 * timing, observers, diagnostics, allocation, logging, or environment.
 */

#include "op_addm_lk_smem_absolute.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define ADDM_LK_SMEM_ABSOLUTE_FIRST_WORD UINT16_C(0x6BF8)
#define ADDM_LK_SMEM_ABSOLUTE_SELECTOR UINT8_C(0x78)

typedef struct addm_lk_smem_absolute_fields {
    uint8_t indirect;
    uint8_t selector;
    uint16_t absolute;
    uint16_t lk;
} addm_lk_smem_absolute_fields;

static ti_c54x_op_status addm_lk_smem_absolute_values(
    const addm_lk_smem_absolute_fields *fields,
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
        fields->selector != ADDM_LK_SMEM_ABSOLUTE_SELECTOR) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    /* OVM/SXM unknownness gates the saturation before any bus
     * traffic (ADDM indirect precedent). */
    if (!input->status_ovm_known || !input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_ovm > UINT8_C(1) ||
        input->status_sxm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = fields->lk;
    staged.payload.memory.rmw_operation =
        (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_ADD;
    staged.payload.memory.complete = NULL;
    staged.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_ATOMIC_ABSOLUTE_WORD16;
    staged.payload.memory.address_spec.atomic_absolute.absolute =
        fields->absolute;
    staged.payload.memory.address_spec.atomic_absolute.reserved_half =
        UINT16_C(0);
    staged.payload.memory.address_spec.atomic_absolute.reserved_word0 =
        UINT32_C(0);
    staged.payload.memory.address_spec.atomic_absolute.reserved_word1 =
        UINT32_C(0);
    staged.payload.memory.address_spec.atomic_absolute.reserved_word2 =
        UINT32_C(0);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status addm_lk_smem_absolute_extract(
    const ti_c54x_decoded_instruction *instruction,
    addm_lk_smem_absolute_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value selector_field = {0U, 0U};
    ti_c54x_operand_field_value absolute_field = {0U, 0U};
    ti_c54x_operand_field_value lk_field = {0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_ADDM_LK_SMEM_ABSOLUTE ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_ABSOLUTE16_LK ||
        instruction->bundle.program_word_count != UINT8_C(3) ||
        instruction->bundle.program_words[0] !=
            ADDM_LK_SMEM_ABSOLUTE_FIRST_WORD) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_ABSOLUTE16_LK,
            UINT8_C(3), &layout_view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};

        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &indirect_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            indirect_field.width_bits != UINT8_C(1) ||
            indirect_field.raw_value > UINT64_C(1) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &selector_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            selector_field.width_bits != UINT8_C(7) ||
            selector_field.raw_value > UINT64_C(0x7F) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &absolute_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            absolute_field.width_bits != UINT8_C(16) ||
            absolute_field.raw_value > UINT64_C(0xFFFF) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(3), &lk_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            lk_field.width_bits != UINT8_C(16) ||
            lk_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    fields->indirect = (uint8_t)indirect_field.raw_value;
    fields->selector = (uint8_t)selector_field.raw_value;
    fields->absolute = (uint16_t)absolute_field.raw_value;
    fields->lk = (uint16_t)lk_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_addm_lk_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    addm_lk_smem_absolute_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = addm_lk_smem_absolute_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return addm_lk_smem_absolute_values(&fields, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_addm_lk_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    addm_lk_smem_absolute_fields fields;

    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(4) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(7) ||
        operands->values[1].raw_value > UINT64_C(0x7F) ||
        operands->values[2].width_bits != UINT8_C(16) ||
        operands->values[2].raw_value > UINT64_C(0xFFFF) ||
        operands->values[3].width_bits != UINT8_C(16) ||
        operands->values[3].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    fields.indirect = (uint8_t)operands->values[0].raw_value;
    fields.selector = (uint8_t)operands->values[1].raw_value;
    fields.absolute = (uint16_t)operands->values[2].raw_value;
    fields.lk = (uint16_t)operands->values[3].raw_value;
    return addm_lk_smem_absolute_values(&fields, input, effect);
}
