/* Module contract
 * Owns: pure Xmem WORD16 SRCCD conditional-store memory-RMW effects over
 * one BRC source value, one Xmod operand, one Xar operand, and one 4-bit
 * accumulator condition evaluated by the shared store-owned evaluator
 * (full-40-bit two's-complement signed compare against zero with guard
 * bits included and consumed-input-only knownness). Cond-true stages the
 * BRC value with OVERWRITE and ignores the read word; cond-false stages
 * WRITEBACK so the read word is rewritten and the staged value is
 * ignored. SXM and ASM are provably not inputs: no shift or field
 * depends on them, so they ride hostile and unconsumed.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "op_srccd_xmem_cond.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "op_store_cond_common.h"

static void srccd_xmem_cond_stage(uint8_t x_mode, uint8_t x_ar,
                                  uint16_t value, bool taken,
                                  ti_c54x_op_effect *effect)
{
    *effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind =
                    TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY,
                .dual = {.x_mode = x_mode,
                         .x_ar = x_ar,
                         .y_mode = x_mode,
                         .y_ar = x_ar,
                         .reserved_word0 = UINT32_C(0),
                         .reserved_word1 = UINT32_C(0),
                         .reserved_word2 = UINT32_C(0),
                         .reserved_word3 = UINT32_C(0)}},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = value,
            .rmw_operation = (uint8_t)(taken
                                           ? TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE
                                           : TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK),
            .complete = NULL}};
}

static ti_c54x_op_status srccd_xmem_cond_values(
    uint8_t select, uint8_t x_mode, uint8_t x_ar, uint8_t cond4,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_status status;
    bool taken = false;

    /* S is pinned 1 by the SRCCD first-word mask; a cleared select has no
     * encoding. Xar arrives absolute (AR2-AR5); the 4-bit condition range
     * is vacuous but keeps the values core total. */
    if (select != UINT8_C(1) || x_mode > UINT8_C(3) ||
        x_ar < UINT8_C(2) || x_ar > UINT8_C(5) || cond4 > UINT8_C(15)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = ti_c54x_store_cond_evaluate(cond4, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (!input->brc_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    srccd_xmem_cond_stage(x_mode, x_ar, input->brc_register_raw16, taken,
                          &staged);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_srccd_xmem_cond(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value select_field = {0U, 0U};
    ti_c54x_operand_field_value xmod_field = {0U, 0U};
    ti_c54x_operand_field_value xar_field = {0U, 0U};
    ti_c54x_operand_field_value cond_field = {0U, 0U};

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_SRCCD_XMEM_COND);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_SRCCD_XMEM_COND ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_SELECT_DUAL_XMOD_XAR_COND4 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_BRC_REGISTER ||
        instruction->id != TI_C54X_OPCODE_ID_SRCCD_XMEM_COND ||
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
                &layout_view, &words, UINT8_C(0), &select_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &xmod_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &xar_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(3), &cond_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            select_field.width_bits != UINT8_C(1) ||
            xmod_field.width_bits != UINT8_C(2) ||
            xar_field.width_bits != UINT8_C(2) ||
            cond_field.width_bits != UINT8_C(4) ||
            select_field.raw_value > UINT64_C(1) ||
            xmod_field.raw_value > UINT64_C(3) ||
            xar_field.raw_value > UINT64_C(3) ||
            cond_field.raw_value > UINT64_C(15)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    return srccd_xmem_cond_values((uint8_t)select_field.raw_value,
                                  (uint8_t)xmod_field.raw_value,
                                  (uint8_t)(xar_field.raw_value +
                                            UINT64_C(2)),
                                  (uint8_t)cond_field.raw_value, input,
                                  effect);
}

ti_c54x_op_status ti_c54x_trusted_op_srccd_xmem_cond(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(4) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(2) ||
        operands->values[1].raw_value > UINT64_C(3) ||
        operands->values[2].width_bits != UINT8_C(2) ||
        operands->values[2].raw_value > UINT64_C(3) ||
        operands->values[3].width_bits != UINT8_C(4) ||
        operands->values[3].raw_value > UINT64_C(15)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return srccd_xmem_cond_values((uint8_t)operands->values[0].raw_value,
                                  (uint8_t)operands->values[1].raw_value,
                                  (uint8_t)(operands->values[2].raw_value +
                                            UINT64_C(2)),
                                  (uint8_t)operands->values[3].raw_value,
                                  input, effect);
}
