/* Module contract
 * Owns: checked and trusted pure BITT Smem semantics (TI SPRU172C
 * Sec4 BITT, p. 4-23: TC = Smem(15 - T(3-0))) over direct page-0 and
 * standard-indirect MOD0--MOD3 reads through the memory effect. The
 * WORD16 Smem read stages gated on a known T register with no T
 * load; the read-value completion stages one TC STATUS_BIT_WRITE to
 * ST0 bit 12 and no other status. Exotic indirect modes (mode bits
 * above MOD3) defer as INVALID_OPERAND.
 * Does not own: catalog rows, operand-layout definitions, decode,
 * dispatch binding, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "op_bitt_smem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/memory/memory_effect.h"

#define BITT_SMEM_FIRST_WORD_MASK UINT16_C(0xFF00)
#define BITT_SMEM_FIRST_WORD_VALUE UINT16_C(0x3400)

static ti_c54x_op_status bitt_smem_validate_fields(
    uint8_t indirect, uint8_t addr7)
{
    if (indirect > UINT8_C(1) || addr7 > UINT8_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status bitt_smem_validate_t(
    const ti_c54x_op_input_snapshot *input)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!input->t_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status bitt_smem_extract(
    const ti_c54x_decoded_instruction *instruction,
    uint8_t *indirect,
    uint8_t *addr7)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field;
    ti_c54x_program_words_view words;
    uint8_t index;
    uint8_t got[2] = {0U, 0U};
    static const uint8_t want_width[2] = {1U, 7U};
    static const uint64_t want_max[2] = {1U, 0x7FU};

    if (instruction == NULL || indirect == NULL || addr7 == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_BITT_SMEM ||
        instruction->operand_layout != TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7 ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   BITT_SMEM_FIRST_WORD_MASK) != BITT_SMEM_FIRST_WORD_VALUE) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7, UINT8_C(1),
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words.program_words = instruction->bundle.program_words;
    words.program_word_count = instruction->bundle.program_word_count;
    for (index = UINT8_C(0); index < UINT8_C(2); ++index) {
        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, index, &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != want_width[index] ||
            field.raw_value > want_max[index]) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        got[index] = (uint8_t)field.raw_value;
    }
    *indirect = got[0];
    *addr7 = got[1];
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status bitt_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_op_effect staged = {0};
    uint8_t indirect;
    uint8_t addr7;
    uint8_t bit_code;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = bitt_smem_extract(instruction, &indirect, &addr7);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)indirect;
    (void)addr7;
    status = bitt_smem_validate_t(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    bit_code = (uint8_t)(input->t_register_raw16 & UINT16_C(0x000F));
    staged.kind = TI_C54X_OP_EFFECT_STATUS_BIT_WRITE;
    staged.payload.status_bit_write.target_register =
        TI_C54X_OP_STATUS_REGISTER_ST0;
    staged.payload.status_bit_write.bit_index = UINT8_C(12);
    staged.payload.status_bit_write.raw_value =
        (uint8_t)(((uint16_t)(read_value_word >>
                              (UINT8_C(15) - bit_code)) &
                   UINT16_C(1)) != UINT16_C(0)
                      ? UINT8_C(1)
                      : UINT8_C(0));
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status bitt_smem_stage_read(
    uint8_t indirect, uint8_t addr7, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.complete = bitt_smem_complete;
    staged.payload.memory.load_t_with_read = false;
    if (indirect == UINT8_C(0)) {
        staged.payload.memory.address_spec.kind =
            TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16;
        staged.payload.memory.address_spec.direct_dma7 = addr7;
    } else {
        const uint8_t mode = (uint8_t)(addr7 >> 3);
        const uint8_t arf = (uint8_t)(addr7 & UINT8_C(7));

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

ti_c54x_op_status ti_c54x_op_bitt_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t indirect;
    uint8_t addr7;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = bitt_smem_extract(instruction, &indirect, &addr7);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ti_c54x_op_bitt_smem_values(indirect, addr7, input, effect);
}

ti_c54x_op_status ti_c54x_op_bitt_smem_values(
    uint8_t indirect,
    uint8_t addr7,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_status status;

    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = bitt_smem_validate_fields(indirect, addr7);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = bitt_smem_validate_t(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return bitt_smem_stage_read(indirect, addr7, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_bitt_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint8_t indirect;
    uint8_t addr7;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(2)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[1].width_bits != UINT8_C(7) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].raw_value > UINT64_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    indirect = (uint8_t)operands->values[0].raw_value;
    addr7 = (uint8_t)operands->values[1].raw_value;
    return ti_c54x_op_bitt_smem_values(indirect, addr7, input, effect);
}
