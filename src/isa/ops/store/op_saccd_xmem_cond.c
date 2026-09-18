/* Module contract
 * Owns: pure dual-X Xmem WORD16 SACCD conditional-store memory effects
 * over one S-selected accumulator shifted by (ASM-16) with SXM-gated
 * right-shift fill, one Xmod operand, one Xar operand, and one 4-bit
 * accumulator condition. The stored word is the low 16 of the shifted
 * source: SXM-gating is observable (right shifts exposing the fill in
 * the staged window), so both ASM and SXM are consumed inputs.
 * Cond-true stages a
 * READ with the OVERWRITE transform; cond-false stages the same READ
 * with WRITEBACK, which rewrites the read word and ignores the staged
 * value (TI SPRU172C: Xmem is always read and updated). Condition
 * evaluation is the shared store-owned S48 evaluator.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "op_saccd_xmem_cond.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_store_cond_common.h"

/* Stage the low 16 bits of the accumulator shifted by (ASM-16).
 * ASM is the signed 5-bit count (raw <= 15 non-negative, else raw -
 * 32), so the effective shift is always <= -1: a right shift by
 * (16-ASM) with arithmetic fill under SXM=1 and zero fill under
 * SXM=0. TI SPRU172C p. 4-153 pins this: A FF_FE00_4321 with
 * ASM=01 stages 0xFC00. */
static uint16_t saccd_shifted_store(uint64_t source_raw40, uint8_t asm_value,
                                    uint8_t sxm)
{
    const int32_t asm_signed = asm_value <= UINT8_C(15)
                                   ? (int32_t)asm_value
                                   : (int32_t)asm_value - INT32_C(32);
    const uint64_t right =
        (uint64_t)(INT32_C(16) - asm_signed);
    ti_c54x_u40_t shifted;

    if (sxm != UINT8_C(0)) {
        shifted =
            ti_c54x_u40_arithmetic_right_shift(source_raw40, right);
    } else {
        shifted = ti_c54x_u40_mask(source_raw40 >> right);
    }
    return (uint16_t)(shifted & UINT64_C(0xFFFF));
}

static void saccd_memory_rmw(uint8_t x_mode, uint8_t x_ar, uint16_t value,
                             uint8_t rmw_operation, ti_c54x_op_effect *effect)
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
            .rmw_operation = rmw_operation,
            .complete = NULL}};
}

static ti_c54x_op_status saccd_values(
    uint8_t source, uint8_t x_mode, uint8_t x_ar, uint8_t cond4,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40;
    bool source_known;
    bool taken;
    ti_c54x_op_status cond_status;
    ti_c54x_op_effect staged = {0};

    if (source > UINT8_C(1) || x_mode > UINT8_C(3) ||
        x_ar < UINT8_C(2) || x_ar > UINT8_C(5) || cond4 > UINT8_C(15)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (source == UINT8_C(0)) {
        source_raw40 = input->accumulator_a_raw40;
        source_known = input->accumulator_a_known;
    } else {
        source_raw40 = input->accumulator_b_raw40;
        source_known = input->accumulator_b_known;
    }
    if (!source_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (source_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (!input->status_asm_known || !input->status_sxm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_asm > UINT8_C(31) ||
        input->status_sxm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    cond_status = ti_c54x_store_cond_evaluate(cond4, input, &taken);
    if (cond_status != TI_C54X_OP_STATUS_OK) {
        return cond_status;
    }
    saccd_memory_rmw(x_mode, x_ar,
                     saccd_shifted_store(source_raw40, input->status_asm,
                                         input->status_sxm),
                     taken ? (uint8_t)
                                 TI_C54X_OP_MEMORY_RMW_OPERATION_OVERWRITE
                           : (uint8_t)
                                 TI_C54X_OP_MEMORY_RMW_OPERATION_WRITEBACK,
                     &staged);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_saccd_xmem_cond(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value xmod_field = {0U, 0U};
    ti_c54x_operand_field_value xar_field = {0U, 0U};
    ti_c54x_operand_field_value cond_field = {0U, 0U};

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_SACCD_XMEM_COND);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_SACCD_XMEM_COND ||
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
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_SXM_ASM ||
        instruction->id != TI_C54X_OPCODE_ID_SACCD_XMEM_COND ||
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
                &layout_view, &words, UINT8_C(1), &xmod_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(2), &xar_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(3), &cond_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            source_field.width_bits != UINT8_C(1) ||
            xmod_field.width_bits != UINT8_C(2) ||
            xar_field.width_bits != UINT8_C(2) ||
            cond_field.width_bits != UINT8_C(4) ||
            source_field.raw_value > UINT64_C(1) ||
            xmod_field.raw_value > UINT64_C(3) ||
            xar_field.raw_value > UINT64_C(3) ||
            cond_field.raw_value > UINT64_C(15)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    return saccd_values((uint8_t)source_field.raw_value,
                        (uint8_t)xmod_field.raw_value,
                        (uint8_t)(xar_field.raw_value + UINT64_C(2)),
                        (uint8_t)cond_field.raw_value, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_saccd_xmem_cond(
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
    return saccd_values((uint8_t)operands->values[0].raw_value,
                        (uint8_t)operands->values[1].raw_value,
                        (uint8_t)(operands->values[2].raw_value +
                                  UINT64_C(2)),
                        (uint8_t)operands->values[3].raw_value, input,
                        effect);
}
