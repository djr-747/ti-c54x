/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for MVMM.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution, buses, CPU or architectural state,
 * sequencing, timing, observers, diagnostics, allocation,
 * logging, environment, or callbacks.
 */

#include "op_mvmm.h"

#include <stddef.h>
#include <stdint.h>

#include "../../decode/operand_layout.h"
#include "../../decode/operand_field.h"

/* DATA-mapped address of AR0: MMR codes 0-8 (AR0-AR7, SP) read
 * and store through 0x10-0x18 (TI SPRU172C CPU memory map). */
#define MVMM_MMR_BASE UINT16_C(0x0010)
/* Highest legal MMR code: AR0-AR7 (0-7) plus SP (8). */
#define MVMM_MMR_MAX UINT8_C(8)

static ti_c54x_op_status mvmm_validate_instruction(
    const ti_c54x_decoded_instruction *instruction, uint8_t *mmrx,
    uint8_t *mmry)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field;
    ti_c54x_operand_field_value destination_field;

    if (instruction == NULL || mmrx == NULL || mmry == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MVMM);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_MVMM ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->id != TI_C54X_OPCODE_ID_MVMM ||
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
                &layout_view, &words, UINT8_C(0), &source_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1),
                &destination_field) != TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            source_field.width_bits != UINT8_C(4) ||
            source_field.raw_value > UINT64_C(0x0F) ||
            destination_field.width_bits != UINT8_C(4) ||
            destination_field.raw_value > UINT64_C(0x0F)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
        /* Structural extraction succeeded: illegal MMR codes are
         * an operand problem, reported identically by both
         * leaves so the dispatch seam stays in parity. */
        if (source_field.raw_value > (uint64_t)MVMM_MMR_MAX ||
            destination_field.raw_value > (uint64_t)MVMM_MMR_MAX) {
            return TI_C54X_OP_STATUS_INVALID_OPERAND;
        }
    }

    *mmrx = (uint8_t)source_field.raw_value;
    *mmry = (uint8_t)destination_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static void mvmm_mmr_memory_read(uint8_t mmrx, uint8_t mmry,
                                 ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind =
                    TI_C54X_OP_MEMORY_ADDRESS_ABSOLUTE_DATA_WORD16,
                .absolute_data = {
                    .address = (uint16_t)(MVMM_MMR_BASE + mmrx),
                    .mmr_sidecar_kind = (uint8_t)
                        TI_C54X_OP_MEMORY_MMR_SIDECAR_STORE_WITH_READ,
                    .mmr_sidecar_number =
                        (uint8_t)(MVMM_MMR_BASE + mmry)}},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .complete = NULL}};
}

ti_c54x_op_status ti_c54x_op_mvmm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t mmrx;
    uint8_t mmry;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = mvmm_validate_instruction(instruction, &mmrx, &mmry);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }

    {
        ti_c54x_op_effect staged = {0};
        mvmm_mmr_memory_read(mmrx, mmry, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_mvmm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    uint8_t mmrx;
    uint8_t mmry;

    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(4) ||
        operands->values[0].raw_value > UINT64_C(0x0F) ||
        operands->values[1].width_bits != UINT8_C(4) ||
        operands->values[1].raw_value > UINT64_C(0x0F) ||
        operands->values[0].raw_value > (uint64_t)MVMM_MMR_MAX ||
        operands->values[1].raw_value > (uint64_t)MVMM_MMR_MAX) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    mmrx = (uint8_t)operands->values[0].raw_value;
    mmry = (uint8_t)operands->values[1].raw_value;

    {
        ti_c54x_op_effect staged = {0};
        mvmm_mmr_memory_read(mmrx, mmry, &staged);
        *effect = staged;
    }
    return TI_C54X_OP_STATUS_OK;
}
