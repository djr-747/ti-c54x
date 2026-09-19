/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for READA
 * direct and indirect. Does not own: opcode inventory,
 * operand-layout definitions, address resolution, buses, CPU or
 * architectural state, sequencing, timing, observers,
 * diagnostics, allocation, logging, environment, or callbacks.
 */

#include "op_reada.h"

#include <stddef.h>
#include <stdint.h>

#include "../../decode/operand_layout.h"
#include "../../decode/operand_field.h"
#include "foundation/arithmetic40.h"

typedef struct reada_fields {
    uint8_t indirect;
    uint8_t cookie;
} reada_fields;

static ti_c54x_op_status reada_extract(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id, uint8_t expected_indirect,
    reada_fields *fields)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value indirect_field;
    ti_c54x_operand_field_value cookie_field;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(expected_id);
    if (metadata == NULL || metadata->id != expected_id ||
        instruction->id != expected_id ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
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
                &layout_view, &words, UINT8_C(0), &indirect_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            indirect_field.width_bits != UINT8_C(1) ||
            indirect_field.raw_value > UINT64_C(1) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &cookie_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            cookie_field.width_bits != UINT8_C(7) ||
            cookie_field.raw_value > UINT64_C(0x7F)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    if (indirect_field.raw_value != (uint64_t)expected_indirect) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    fields->indirect = (uint8_t)indirect_field.raw_value;
    fields->cookie = (uint8_t)cookie_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status reada_trusted_fields(
    const ti_c54x_trusted_operand_values *operands,
    uint8_t expected_indirect, reada_fields *fields)
{
    if (operands == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(7) ||
        operands->values[1].raw_value > UINT64_C(0x7F) ||
        operands->values[0].raw_value !=
            (uint64_t)expected_indirect) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    fields->indirect = (uint8_t)operands->values[0].raw_value;
    fields->cookie = (uint8_t)operands->values[1].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

/* Gate accumulator A knownness and canonical shape: PAR <- A
 * resolves at prepare time, so unknown or non-canonical A
 * rejects here before any bus traffic. */
static ti_c54x_op_status reada_gate_accumulator_a(
    const ti_c54x_op_input_snapshot *input)
{
    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!input->accumulator_a_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->accumulator_a_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static void reada_stage_pair(const reada_fields *fields, uint8_t mode,
                             uint8_t arf, uint8_t dma,
                             ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind =
                    TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_A_PAIR_WORD16,
                .smem_program_a = {
                    .indirect = fields->indirect,
                    .smem_mode = mode,
                    .smem_arf = arf,
                    .smem_dma7 = dma}},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .complete = NULL}};
}

ti_c54x_op_status ti_c54x_op_reada_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    reada_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = reada_extract(instruction, TI_C54X_OPCODE_ID_READA_DIRECT,
                           UINT8_C(0), &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* DP-relative direct: the DP page freezes at resolution, so
     * unknown DP rejects here before any bus traffic. */
    if (!input->status_dp_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    status = reada_gate_accumulator_a(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }

    {
        ti_c54x_op_effect staged = {0};
        reada_stage_pair(&fields, UINT8_C(0), UINT8_C(0),
                         fields.cookie, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_reada_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    reada_fields fields;
    ti_c54x_op_status status;

    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (reada_trusted_fields(operands, UINT8_C(0), &fields) !=
        TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (!input->status_dp_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    status = reada_gate_accumulator_a(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }

    {
        ti_c54x_op_effect staged = {0};
        reada_stage_pair(&fields, UINT8_C(0), UINT8_C(0),
                         fields.cookie, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_reada_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    reada_fields fields;
    ti_c54x_op_status status;
    uint8_t mode;
    uint8_t arf;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = reada_extract(instruction, TI_C54X_OPCODE_ID_READA_SMEM,
                           UINT8_C(1), &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    /* mode4 = cookie >> 3, arf = cookie & 7 (LTD precedent):
     * modes 4+ need extension words the one-word bundle lacks. */
    mode = (uint8_t)((fields.cookie >> 3U) & UINT8_C(0x0F));
    arf = (uint8_t)(fields.cookie & UINT8_C(0x07));
    if (mode > UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = reada_gate_accumulator_a(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }

    {
        ti_c54x_op_effect staged = {0};
        reada_stage_pair(&fields, mode, arf, UINT8_C(0), &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_reada_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    reada_fields fields;
    ti_c54x_op_status status;
    uint8_t mode;
    uint8_t arf;

    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (reada_trusted_fields(operands, UINT8_C(1), &fields) !=
        TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    mode = (uint8_t)((fields.cookie >> 3U) & UINT8_C(0x0F));
    arf = (uint8_t)(fields.cookie & UINT8_C(0x07));
    if (mode > UINT8_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = reada_gate_accumulator_a(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }

    {
        ti_c54x_op_effect staged = {0};
        reada_stage_pair(&fields, mode, arf, UINT8_C(0), &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}
