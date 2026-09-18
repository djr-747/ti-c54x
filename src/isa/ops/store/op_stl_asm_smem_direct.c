/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the row-less STL
 * src,ASM,Smem direct-slice WORD16 store of bits 15:0 of the ASM-shifted
 * selected accumulator to page-0 DATA Smem (first word 0x8400 under mask
 * 0xFE80, S@8 selects A/B, I@7 pinned 0, 7-bit direct address@6:0).
 * The checked leaf validates the catalog/decode contract and extracts
 * the select, I, plus address fields; the self-contained shift core
 * applies the signed 5-bit ASM count (left logically, right by SXM:
 * arithmetic under sxm=1, logical zero-fill under sxm=0). Both paths
 * consume only copied accumulator, ASM, and SXM knownness/values and
 * stage one direct memory-write effect with a null completion and no
 * delay flag. A set I bit rejects as INVALID_OPERAND: the indirect STL
 * half is deferred. Borrowed instruction, operand, input, and effect
 * objects are used for one call only; no pointer is retained and failed
 * calls leave the caller effect unchanged.
 * Does not own: opcode inventory, operand layouts, accumulator/status
 * state or collection, address resolution, buses, CPU/state mutation,
 * sequencing, timing, observers, diagnostics, allocation, logging, or
 * environment.
 */

#include "op_stl_asm_smem_direct.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct stl_asm_smem_direct_fields {
    uint8_t select;
    uint8_t indirect;
    uint8_t addr7;
} stl_asm_smem_direct_fields;

static ti_c54x_u40_t stl_asm_smem_direct_shift(ti_c54x_u40_t source_raw40,
                                              int32_t shift, uint8_t sxm)
{
    if (shift >= INT32_C(0)) {
        return ti_c54x_u40_logical_left_shift(source_raw40,
                                              (uint64_t)shift);
    }
    if (sxm != UINT8_C(0)) {
        return ti_c54x_u40_arithmetic_right_shift(source_raw40,
                                                  (uint64_t)(-shift));
    }
    return ti_c54x_u40_mask(source_raw40 >> (uint64_t)(-shift));
}

static void stl_asm_smem_direct_memory_write(uint8_t addr7, uint16_t value,
                                             ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16,
                .direct_dma7 = addr7},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .value_word = value,
            .complete = NULL}};
}

static ti_c54x_op_status stl_asm_smem_direct_values(
    uint8_t select, uint8_t indirect, uint8_t addr7,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40;
    bool source_known;
    int32_t shift;
    ti_c54x_u40_t shifted;
    ti_c54x_op_effect staged = {0};

    if (select > UINT8_C(1) || indirect > UINT8_C(1) ||
        addr7 > UINT8_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (indirect != UINT8_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (select == UINT8_C(0)) {
        source_raw40 = input->accumulator_a_raw40;
        source_known = input->accumulator_a_known;
    } else {
        source_raw40 = input->accumulator_b_raw40;
        source_known = input->accumulator_b_known;
    }
    if (!source_known || !input->status_asm_known ||
        !input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (source_raw40 > TI_C54X_U40_MASK ||
        input->status_asm > UINT8_C(31) ||
        input->status_sxm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    shift = input->status_asm <= UINT8_C(15)
                ? (int32_t)input->status_asm
                : (int32_t)input->status_asm - INT32_C(32);
    shifted = stl_asm_smem_direct_shift(source_raw40, shift,
                                        input->status_sxm);
    stl_asm_smem_direct_memory_write(addr7, (uint16_t)shifted, &staged);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status stl_asm_smem_direct_extract(
    const ti_c54x_decoded_instruction *instruction,
    stl_asm_smem_direct_fields *fields)
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
        TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT);
    if (metadata == NULL ||
        metadata->id != TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT ||
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
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_ASM ||
        instruction->id != TI_C54X_OPCODE_ID_STL_ASM_SMEM_DIRECT ||
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
        /* The indirect half of the STL ASM first-word family (top
         * bytes 0x84/0x85 with the I bit set) stays decodable here so
         * the shared staging core rejects it as INVALID_OPERAND;
         * indirect STL addressing is deferred. */
        uint16_t top = (uint16_t)(word & UINT16_C(0xFF00));

        if ((top != UINT16_C(0x8400) && top != UINT16_C(0x8500)) ||
            (word & UINT16_C(0x0080)) == UINT16_C(0)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    fields->select = (uint8_t)select_field.raw_value;
    fields->indirect = (uint8_t)indirect_field.raw_value;
    fields->addr7 = (uint8_t)addr7_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_stl_asm_smem_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    stl_asm_smem_direct_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = stl_asm_smem_direct_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return stl_asm_smem_direct_values(fields.select, fields.indirect,
                                      fields.addr7, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_stl_asm_smem_direct(
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

    return stl_asm_smem_direct_values(
        (uint8_t)operands->values[0].raw_value,
        (uint8_t)operands->values[1].raw_value,
        (uint8_t)operands->values[2].raw_value, input, effect);
}
