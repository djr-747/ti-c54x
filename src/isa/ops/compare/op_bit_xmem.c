/* Module contract
 * Owns: checked and trusted pure BIT Xmem semantics (TI SPRU172C
 * Sec4 BIT, p. 4-21: TC = Xmem(15 - BITC)) over the one-word 0x9600
 * dual-XMOD/XAR/BITC form. The WORD16 DATA READ stages gated on no
 * snapshot field: BITC rides the opcode, and address-register
 * known-ness stays the resolver's gate, as in the Xmem siblings.
 * All four XMOD codes stage, including MOD3 (XMOD 3) indexed and
 * circular commits, which the shared X-only dual resolver owns
 * exactly as in the LD Xmem SHIFT sibling. The read-value
 * completion stages one TC STATUS_BIT_WRITE to ST0 bit 12 and no
 * other status. The completion re-validates the layout, word count,
 * and 0x9600 prefix from the raw first word; XMOD needs no range
 * re-check there because every 2-bit code stages.
 * Does not own: catalog rows, operand-layout definitions, decode,
 * dispatch binding, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_bit_xmem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/memory/memory_effect.h"

#define BIT_XMEM_FIRST_WORD_MASK UINT16_C(0xFF00)
#define BIT_XMEM_FIRST_WORD_VALUE UINT16_C(0x9600)

typedef struct bit_xmem_fields {
    uint8_t x_mode;
    uint8_t x_ar;
    uint8_t bitc;
} bit_xmem_fields;

static ti_c54x_op_status bit_xmem_validate_fields(
    uint8_t x_mode, uint8_t x_ar, uint8_t bitc)
{
    if (x_mode > UINT8_C(3) || x_ar < UINT8_C(2) ||
        x_ar > UINT8_C(5) || bitc > UINT8_C(15)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status bit_xmem_extract(
    const ti_c54x_decoded_instruction *instruction,
    bit_xmem_fields *fields)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    uint8_t index;
    uint8_t got[3] = {0U, 0U, 0U};
    static const uint8_t want_width[3] = {2U, 2U, 4U};
    static const uint64_t want_max[3] = {3U, 3U, 15U};

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_BITC ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   BIT_XMEM_FIRST_WORD_MASK) != BIT_XMEM_FIRST_WORD_VALUE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_BITC, UINT8_C(1),
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < UINT8_C(3); ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != want_width[index] ||
            field.raw_value > want_max[index]) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        got[index] = (uint8_t)field.raw_value;
    }
    fields->x_mode = got[0];
    fields->x_ar = (uint8_t)(got[1] + UINT8_C(2));
    fields->bitc = got[2];
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status bit_xmem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_op_effect staged = {0};
    uint16_t first_word;
    uint8_t bitc;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    (void)input;
    if (instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_BITC ||
        instruction->bundle.program_word_count != UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    first_word = instruction->bundle.program_words[0];
    /* Every 2-bit XMOD code stages (MOD3 included), so only the
     * prefix can fail here; field ranges belong to extraction. */
    if ((uint16_t)(first_word & BIT_XMEM_FIRST_WORD_MASK) !=
        BIT_XMEM_FIRST_WORD_VALUE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    bitc = (uint8_t)(first_word & UINT16_C(0x000F));
    staged.kind = TI_C54X_OP_EFFECT_STATUS_BIT_WRITE;
    staged.payload.status_bit_write.target_register =
        TI_C54X_OP_STATUS_REGISTER_ST0;
    staged.payload.status_bit_write.bit_index = UINT8_C(12);
    staged.payload.status_bit_write.raw_value =
        (uint8_t)(((uint16_t)(read_value_word >>
                              (UINT8_C(15) - bitc)) &
                   UINT16_C(1)) != UINT16_C(0)
                      ? UINT8_C(1)
                      : UINT8_C(0));
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status bit_xmem_stage_read(
    const bit_xmem_fields *fields, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    /* Y self-aliases X: the X-only resolver reads and commits the X
     * side alone through the single-read child, as in the ADD Xmem
     * leaf. */
    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.complete = bit_xmem_complete;
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

ti_c54x_op_status ti_c54x_op_bit_xmem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    bit_xmem_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = bit_xmem_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ti_c54x_op_bit_xmem_values(
        fields.x_mode, fields.x_ar, fields.bitc, input, effect);
}

ti_c54x_op_status ti_c54x_op_bit_xmem_values(
    uint8_t x_mode,
    uint8_t x_ar,
    uint8_t bitc,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    bit_xmem_fields fields;
    ti_c54x_op_status status;

    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    (void)input;
    status = bit_xmem_validate_fields(x_mode, x_ar, bitc);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    fields.x_mode = x_mode;
    fields.x_ar = x_ar;
    fields.bitc = bitc;
    return bit_xmem_stage_read(&fields, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_bit_xmem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    static const uint8_t widths[3] = {2U, 2U, 4U};
    static const uint64_t maxima[3] = {3U, 3U, 15U};
    uint64_t raw[3] = {0U, 0U, 0U};
    uint8_t index;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    for (index = UINT8_C(0); index < UINT8_C(3); ++index) {
        if (operands->values[index].width_bits != widths[index] ||
            operands->values[index].raw_value > maxima[index]) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
        raw[index] = operands->values[index].raw_value;
    }
    return ti_c54x_op_bit_xmem_values(
        (uint8_t)raw[0], (uint8_t)(raw[1] + UINT64_C(2)), (uint8_t)raw[2],
        input, effect);
}
