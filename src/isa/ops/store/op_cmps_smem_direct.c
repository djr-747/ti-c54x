/*
 * Module contract
 *
 * Owns: pure direct-Smem CMPS conditional max-select-store memory
 * effects. TI SPRU172C p. 4-35 pins the semantics: the signed
 * 16-bit maximum of src(31:16) and src(15:0) stores to Smem,
 * TRN shifts left with the taken bit into TRN(0), and TC takes
 * the taken bit. Only bits 31:0 participate; guard bits are
 * excluded. The deferred indirect shape (I=1) rejects
 * transactionally. The completion carries the taken outcome
 * into the TRN_SHIFT_WRITE follow-on; the sequencer commits
 * TRN and TC from those copied values.
 * Does not own: catalog rows, operand extraction, effect
 * staging, CPU/state mutation, sequencing, timing, buses,
 * memory, diagnostics, allocation, logging, or environment.
 */

#include "op_cmps_smem_direct.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct cmps_smem_direct_fields {
    uint8_t select;
    uint8_t indirect;
    uint8_t addr7;
} cmps_smem_direct_fields;

/* Signed 16-bit maximum over bits 31:0; guard bits excluded.
 * Taken means the high half strictly wins; ties store the low
 * half with the untaken TRN/TC disposition. */
static bool cmps_high_wins(uint32_t source_low32)
{
    int16_t high = (int16_t)(uint16_t)(source_low32 >> 16U);
    int16_t low = (int16_t)(uint16_t)source_low32;

    return high > low;
}

static ti_c54x_op_status cmps_source_raw40(
    uint8_t select, const ti_c54x_op_input_snapshot *input,
    ti_c54x_u40_t *source_raw40)
{
    bool source_known;

    if (select == UINT8_C(0)) {
        *source_raw40 = input->accumulator_a_raw40;
        source_known = input->accumulator_a_known;
    } else {
        *source_raw40 = input->accumulator_b_raw40;
        source_known = input->accumulator_b_known;
    }
    if (!source_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (*source_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status cmps_trn_shift_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_u40_t source_raw40 = UINT64_C(0);
    ti_c54x_op_status status;
    bool taken;
    uint16_t trn_word;

    (void)read_value_word;
    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    /* The frozen snapshot carries the same validated inputs the
     * leaf consumed, so the taken outcome recomputes identically
     * here without any leaf-to-completion side channel. The S bit
     * is single-bit by construction. */
    status = cmps_source_raw40(
        (uint8_t)((instruction->bundle.program_words[0] >> 8U) &
                  UINT16_C(1)),
        input, &source_raw40);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (!input->trn_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    taken = cmps_high_wins((uint32_t)source_raw40);
    trn_word = (uint16_t)(((uint16_t)(input->trn_register_raw16 << 1)) |
                          (taken ? UINT16_C(0) : UINT16_C(1)));
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_TRN_SHIFT_WRITE,
        .payload.trn_shift_write = {
            .trn_word = trn_word,
            .tc_bit = taken ? UINT8_C(0) : UINT8_C(1)}};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status cmps_smem_direct_values(
    uint8_t select, uint8_t indirect, uint8_t addr7,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40 = UINT64_C(0);
    ti_c54x_op_status status;
    bool taken;
    int16_t high;
    int16_t low;
    ti_c54x_op_effect staged = {0};

    if (select > UINT8_C(1) || indirect != UINT8_C(0) ||
        addr7 > UINT8_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = cmps_source_raw40(select, input, &source_raw40);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (!input->trn_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    taken = cmps_high_wins((uint32_t)source_raw40);
    high = (int16_t)(uint16_t)((uint32_t)source_raw40 >> 16U);
    low = (int16_t)(uint16_t)(uint32_t)source_raw40;

    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16;
    staged.payload.memory.address_spec.direct_dma7 = addr7;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    staged.payload.memory.value_word =
        (uint16_t)(taken ? high : low);
    staged.payload.memory.complete = cmps_trn_shift_complete;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status cmps_smem_direct_extract(
    const ti_c54x_decoded_instruction *instruction,
    cmps_smem_direct_fields *fields)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value select_field = {0U, 0U};
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value addr7_field = {0U, 0U};
    uint16_t word;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_CMPS_SMEM_DIRECT);
    if (metadata == NULL ||
        metadata->id != TI_C54X_OPCODE_ID_CMPS_SMEM_DIRECT ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_TRN_REGISTER ||
        instruction->id != TI_C54X_OPCODE_ID_CMPS_SMEM_DIRECT ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};

        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &select_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &indirect_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &addr7_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            select_field.width_bits != UINT8_C(1) ||
            select_field.raw_value > UINT64_C(1) ||
            indirect_field.width_bits != UINT8_C(1) ||
            indirect_field.raw_value > UINT64_C(1) ||
            addr7_field.width_bits != UINT8_C(7) ||
            addr7_field.raw_value > UINT64_C(0x7F)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    word = instruction->bundle.program_words[0];
    if ((uint16_t)(word & metadata->first_word_mask) !=
        metadata->first_word_value) {
        /* The indirect half of the CMPS first-word family (base 0x8E00
         * with the I bit set, for either select) stays decodable here
         * so the shared staging core rejects it as INVALID_OPERAND; no
         * CMPS indirect rows exist yet. */
        if ((uint16_t)(word & UINT16_C(0xFE00)) != UINT16_C(0x8E00) ||
            (word & UINT16_C(0x0080)) == UINT16_C(0)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    fields->select = (uint8_t)select_field.raw_value;
    fields->indirect = (uint8_t)indirect_field.raw_value;
    fields->addr7 = (uint8_t)addr7_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_cmps_smem_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    cmps_smem_direct_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = cmps_smem_direct_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return cmps_smem_direct_values(fields.select, fields.indirect,
                                   fields.addr7, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_cmps_smem_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(3) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(1) ||
        operands->values[1].raw_value > UINT64_C(1) ||
        operands->values[2].width_bits != UINT8_C(7) ||
        operands->values[2].raw_value > UINT64_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    return cmps_smem_direct_values((uint8_t)operands->values[0].raw_value,
                                   (uint8_t)operands->values[1].raw_value,
                                   (uint8_t)operands->values[2].raw_value,
                                   input, effect);
}
