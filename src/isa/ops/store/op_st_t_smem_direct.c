/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the row-less ST T
 * direct-slice WORD16 store of the copied T register to page-0 DATA Smem
 * (first word 0x8C00 under mask 0xFF80, I pinned 0, 7-bit direct
 * address). The checked leaf validates the catalog/decode contract and
 * extracts the I plus address fields; both paths consume only copied T
 * knownness/value and stage one direct memory-write effect with a null
 * completion and no delay flag. A set I bit rejects as INVALID_OPERAND:
 * the indirect half of the ST T first-word family composes through the
 * established ST_T MOD rows, which start at 0x8C80. Borrowed
 * instruction, operand, input, and effect objects are used for one call
 * only; no pointer is retained and failed calls leave the caller effect
 * unchanged.
 * Does not own: opcode inventory, operand layouts, T state or collection,
 * address resolution, buses, CPU/state mutation, sequencing, timing,
 * observers, diagnostics, allocation, logging, or environment.
 */

#include "op_st_t_smem_direct.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct st_t_smem_direct_fields {
    uint8_t indirect;
    uint8_t addr7;
} st_t_smem_direct_fields;

static void st_t_smem_direct_memory_write(uint8_t addr7, uint16_t value,
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

static ti_c54x_op_status st_t_smem_direct_values(
    uint8_t indirect, uint8_t addr7,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (indirect != UINT8_C(0) || addr7 > UINT8_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (!input->t_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }

    st_t_smem_direct_memory_write(addr7, input->t_register_raw16, &staged);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status st_t_smem_direct_extract(
    const ti_c54x_decoded_instruction *instruction,
    st_t_smem_direct_fields *fields)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value indirect_field = {0U, 0U};
    ti_c54x_operand_field_value addr7_field = {0U, 0U};
    uint16_t word;

    if (instruction == NULL || fields == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
    if (metadata == NULL ||
        metadata->id != TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_T_REGISTER ||
        instruction->id != TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT ||
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
                &layout_view, &words, UINT8_C(0), &indirect_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &addr7_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
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
        /* The indirect half of the ST T first-word family (bits 15:8 =
         * 1000_1100 with the I bit set) stays decodable here so the
         * shared staging core rejects it as INVALID_OPERAND; it
         * composes through the established ST_T MOD rows. */
        if ((uint16_t)(word & UINT16_C(0xFF00)) != UINT16_C(0x8C00) ||
            (word & UINT16_C(0x0080)) == UINT16_C(0)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    fields->indirect = (uint8_t)indirect_field.raw_value;
    fields->addr7 = (uint8_t)addr7_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_st_t_smem_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    st_t_smem_direct_fields fields;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = st_t_smem_direct_extract(instruction, &fields);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return st_t_smem_direct_values(fields.indirect, fields.addr7, input,
                                   effect);
}

ti_c54x_op_status ti_c54x_trusted_op_st_t_smem_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(7) ||
        operands->values[1].raw_value > UINT64_C(0x7F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    return st_t_smem_direct_values((uint8_t)operands->values[0].raw_value,
                                   (uint8_t)operands->values[1].raw_value,
                                   input, effect);
}
