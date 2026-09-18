/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the direct-only two-word
 * ST #lk,Smem WORD16 store (TI SPRU172C section ST syntax 3: opcode
 * 0111_0110_0AAAAAAA with a 16-bit lk extension word, execution lk -> Smem,
 * no status bits affected). The checked leaf validates the fixed first-word
 * mask locally (0x7600/0xFF80), requires the ST_LK_SMEM opcode id, validates
 * the INDIRECT_ADDR7_EXTENSION16 layout id and 2-word count, and extracts
 * (indirect, addr7, lk) through the layout set; the trusted leaf takes 3
 * operands with exact widths (1,7,16). Both paths share one values core
 * that stages a fire-and-forget generic WORD16 memory-write effect of the
 * copied lk value with no T traffic and no read-value completion:
 * direct (I=0) writes use the page-0 direct address, indirect (I=1) writes
 * decode ARF/mode from the address cookie and reject modes above MOD3 as
 * INVALID_OPERAND. Borrowed instruction, operand, input, and effect objects
 * are used for one call only; no pointer is retained and failed calls leave
 * the caller effect unchanged.
 * Does not own: opcode inventory, catalog rows, dispatch bindings, operand
 * layouts, address resolution, buses, CPU/state mutation, sequencing,
 * timing, observers, diagnostics, allocation, logging, or environment.
 */

#include "op_st_lk_smem.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define ST_LK_SMEM_FIRST_WORD_VALUE UINT16_C(0x7600)
#define ST_LK_SMEM_FIRST_WORD_MASK UINT16_C(0xFF80)

typedef struct st_lk_smem_fields {
    uint8_t indirect;
    uint8_t addr7;
    uint16_t immediate;
} st_lk_smem_fields;

static ti_c54x_op_status st_lk_smem_values(
    const st_lk_smem_fields *fields, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (fields == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    /* The indirect Smem form is deferred: only I=0 direct composes
     * (indirect MOD0--MOD3 rides the established ST_IMMEDIATE_SMEM
     * row). */
    if (fields->indirect != UINT8_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    staged.payload.memory.value_word = fields->immediate;
    staged.payload.memory.complete = NULL;
    if (fields->indirect == UINT8_C(0)) {
        staged.payload.memory.address_spec.kind =
            TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16;
        staged.payload.memory.address_spec.direct_dma7 = fields->addr7;
    } else {
        const uint8_t mode = (uint8_t)(fields->addr7 >> 3);
        const uint8_t arf = (uint8_t)(fields->addr7 & UINT8_C(7));

        if (mode > UINT8_C(3)) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        staged.payload.memory.address_spec.kind =
            TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16;
        staged.payload.memory.address_spec.mode =
            (ti_c54x_indirect_single_mode)mode;
        staged.payload.memory.address_spec.arf = arf;
    }
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_lk_smem_extract(
    const ti_c54x_decoded_instruction *instruction,
    st_lk_smem_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value addr7_field = {0U, 0U};
    ti_c54x_operand_field_value lk_field = {0U, 0U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_ST_LK_SMEM ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_EXTENSION16 ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   ST_LK_SMEM_FIRST_WORD_MASK) !=
            ST_LK_SMEM_FIRST_WORD_VALUE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_EXTENSION16, UINT8_C(2),
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
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
                &layout_view, &words, UINT8_C(1), &addr7_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            addr7_field.width_bits != UINT8_C(7) ||
            addr7_field.raw_value > UINT64_C(0x7F) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &lk_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            lk_field.width_bits != UINT8_C(16) ||
            lk_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    fields->indirect = (uint8_t)indirect_field.raw_value;
    fields->addr7 = (uint8_t)addr7_field.raw_value;
    fields->immediate = (uint16_t)lk_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_st_lk_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    st_lk_smem_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = st_lk_smem_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)input;
    return st_lk_smem_values(&fields, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_st_lk_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    st_lk_smem_fields fields;

    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(3) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(7) ||
        operands->values[1].raw_value > UINT64_C(0x7F) ||
        operands->values[2].width_bits != UINT8_C(16) ||
        operands->values[2].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    fields.indirect = (uint8_t)operands->values[0].raw_value;
    fields.addr7 = (uint8_t)operands->values[1].raw_value;
    fields.immediate = (uint16_t)operands->values[2].raw_value;
    (void)input;
    return st_lk_smem_values(&fields, effect);
}
