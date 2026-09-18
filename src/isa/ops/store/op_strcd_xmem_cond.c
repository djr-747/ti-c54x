/* Module contract
 * Owns: pure Xmem WORD16 STRCD conditional-store memory-read effects
 * over the T register source, one Xmod operand, one Xar operand,
 * and one 4-bit accumulator condition (TI SPRU172C Sec4 STRCD,
 * p. 4-186: 1001_1100_XXXX_COND, T source, one word, one cycle).
 * Cond-true stages the T word under OVERWRITE and ignores the read
 * word; cond-false stages an ignored zero word under WRITEBACK and
 * the root rewrites the read word, exactly the TI read-always /
 * update-always execution. The T source is staged unshifted: no
 * ASM or SXM input is consumed. S is fixed zero (S=1 selects
 * SRCCD). Unknown T and unknown selected accumulator reject;
 * the unselected accumulator may be unknown and the four
 * unassigned cond nibbles reject.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, logging, environment, or
 * callbacks.
 */

#include "op_strcd_xmem_cond.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/memory/memory_effect.h"
#include "op_store_cond_common.h"

static void strcd_stage_rmw_read(uint8_t x_mode, uint8_t x_ar,
                                 bool taken, uint16_t t_word,
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
            .load_t_with_read = false,
            .write_smem_delay = false,
            .value_word = taken ? t_word : UINT16_C(0),
            .rmw_operation = taken ? (uint8_t)
                TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE
                                   : (uint8_t)
                TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK,
            .transfer_reserved = UINT8_C(0),
            .complete = NULL}};
}

static ti_c54x_op_status strcd_values(
    uint8_t select, uint8_t x_mode, uint8_t x_ar, uint8_t cond4,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};
    ti_c54x_op_status status;
    bool taken = false;

    /* S is fixed zero by the STRCD first-word mask (S=1 selects the
     * SRCCD row); the trusted entry enforces it directly since it
     * takes operands without a decode round-trip. */
    if (select != UINT8_C(0) || x_mode > UINT8_C(3) ||
        x_ar < UINT8_C(2) || x_ar > UINT8_C(5) ||
        cond4 > UINT8_C(15)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (!input->t_register_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    status = ti_c54x_store_cond_evaluate(cond4, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    strcd_stage_rmw_read(x_mode, x_ar, taken, input->t_register_raw16,
                         &staged);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_strcd_xmem_cond(
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
        TI_C54X_OPCODE_ID_STRCD_XMEM_COND);
    if (metadata == NULL ||
        metadata->id != TI_C54X_OPCODE_ID_STRCD_XMEM_COND ||
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
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_T_REGISTER ||
        instruction->id != TI_C54X_OPCODE_ID_STRCD_XMEM_COND ||
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

    return strcd_values((uint8_t)select_field.raw_value,
                        (uint8_t)xmod_field.raw_value,
                        (uint8_t)(xar_field.raw_value + UINT64_C(2)),
                        (uint8_t)cond_field.raw_value, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_strcd_xmem_cond(
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
    return strcd_values((uint8_t)operands->values[0].raw_value,
                        (uint8_t)operands->values[1].raw_value,
                        (uint8_t)(operands->values[2].raw_value +
                                  UINT64_C(2)),
                        (uint8_t)operands->values[3].raw_value, input,
                        effect);
}
