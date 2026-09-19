/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the LDU
 * Smem,dst direct-slice WORD16 load of DP-relative DATA Smem into
 * accumulator A or B (first word 0x1200 under mask 0xFE80, D at
 * bit 8 selects A/B, I pinned 0, 7-bit direct address). The
 * checked leaf validates the catalog/decode contract and extracts
 * the select, I, plus address fields; both paths gate DP
 * knownness (UNKNOWN_INPUT) and stage one direct memory-read
 * effect whose static completion zero-extends the read WORD16
 * into a canonical 40-bit accumulator write (SXM is neither
 * observed nor interpreted). A set I bit rejects as
 * INVALID_OPERAND and a select above 1 rejects as
 * INVALID_OPERAND: the I=1 half routes to the S34 indirect
 * leaves, so it stays decodable here for transactional
 * rejection of misroutes. Borrowed instruction, input, and
 * effect objects are used for one call only; no pointer is
 * retained and failed calls leave the caller effect unchanged.
 * Does not own: opcode inventory, operand layouts, accumulator state
 * or collection, address resolution, buses, CPU/state mutation,
 * sequencing, timing, observers, diagnostics, allocation, logging, or
 * environment.
 */

#include "op_ldu_direct.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct ldu_direct_fields {
    uint8_t destination;
    uint8_t indirect;
    uint8_t addr7;
} ldu_direct_fields;

static ti_c54x_op_status ldu_direct_complete_values(
    uint8_t destination,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    const ti_c54x_op_effect staged = {
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = (ti_c54x_u40_t)read_value_word,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE}}};

    if (destination > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ldu_direct_extract(
    const ti_c54x_decoded_instruction *instruction,
    ldu_direct_fields *fields)
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
        TI_C54X_OPCODE_ID_LDU_DIRECT);
    if (metadata == NULL ||
        metadata->id != TI_C54X_OPCODE_ID_LDU_DIRECT ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_STATUS_DP ||
        instruction->id != TI_C54X_OPCODE_ID_LDU_DIRECT ||
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
        /* The indirect half of the LDU first-word family (base 0x1200
         * with the I bit set, for either select) stays decodable here
         * so the shared staging core rejects it as INVALID_OPERAND;
         * the S34 indirect leaves own those rows. */
        if ((uint16_t)(word & UINT16_C(0xFE00)) != UINT16_C(0x1200) ||
            (word & UINT16_C(0x0080)) == UINT16_C(0)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    fields->destination = (uint8_t)select_field.raw_value;
    fields->indirect = (uint8_t)indirect_field.raw_value;
    fields->addr7 = (uint8_t)addr7_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ldu_direct_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ldu_direct_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ldu_direct_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (fields.indirect != UINT8_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return ldu_direct_complete_values(fields.destination,
                                           read_value_word, next_effect);
}

static ti_c54x_op_status ldu_direct_values(
    uint8_t destination, uint8_t indirect, uint8_t addr7,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (destination > UINT8_C(1) || indirect != UINT8_C(0) ||
        addr7 > UINT8_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (!input->status_dp_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_dp > UINT16_C(0x1FF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged.kind = TI_C54X_OP_EFFECT_MEMORY;
    staged.payload.memory.address_spec.kind =
        TI_C54X_OP_MEMORY_ADDRESS_DIRECT_DP_WORD16;
    /* DIRECT_DP reinterprets the shared long/offset fields: the
     * frozen dispatch-time DP page rides `long_offset_word`, the
     * 7-bit offset rides `direct_dma7` (no arm growth). */
    staged.payload.memory.address_spec.long_offset_word = input->status_dp;
    staged.payload.memory.address_spec.direct_dma7 = addr7;
    staged.payload.memory.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    staged.payload.memory.value_word = UINT16_C(0);
    staged.payload.memory.complete = ldu_direct_complete;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_ldu_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ldu_direct_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ldu_direct_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ldu_direct_values(fields.destination, fields.indirect,
                             fields.addr7, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_ldu_direct(
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

    return ldu_direct_values((uint8_t)operands->values[0].raw_value,
                             (uint8_t)operands->values[1].raw_value,
                             (uint8_t)operands->values[2].raw_value,
                             input, effect);
}
