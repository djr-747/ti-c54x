/*
 * Module contract
 *
 * Owns: pure checked/trusted LD src,ASM[,dst] semantics over a copied
 * accumulator source shifted by the signed ASM count with SXM-dependent
 * right shifts, OVM saturation, and sticky destination overflow.
 * Does not own: catalog lookup generation, accumulator/status mutation,
 * CPU sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations.
 */

#include "op_ld_src_asm.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct ld_src_asm_operands {
    uint8_t source;
    uint8_t destination;
} ld_src_asm_operands;

static ti_c54x_op_status ld_src_asm_values(
    uint64_t source_raw40, int32_t shift, uint8_t sxm, uint8_t ovm,
    uint8_t destination, ti_c54x_op_effect *effect)
{
    uint64_t shifted_raw40;
    uint64_t guard_bits;
    uint8_t result_bit31;
    bool overflow;
    bool negative = (source_raw40 & TI_C54X_U40_SIGN_BIT) != UINT64_C(0);
    ti_c54x_op_effect staged = {
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = UINT64_C(0),
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .destination_overflow_action =
                    TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE}}};

    if (shift >= 0) {
        shifted_raw40 = ti_c54x_u40_logical_left_shift(
            source_raw40, (uint64_t)shift);
    } else if (sxm != UINT8_C(0)) {
        shifted_raw40 = ti_c54x_u40_arithmetic_right_shift(
            source_raw40, (uint64_t)(-shift));
    } else {
        shifted_raw40 =
            ti_c54x_u40_mask(source_raw40 >> (uint64_t)(-shift));
    }

    guard_bits = (shifted_raw40 >> UINT64_C(32)) & UINT64_C(0xFF);
    result_bit31 =
        (uint8_t)((shifted_raw40 >> UINT64_C(31)) & UINT64_C(1));
    overflow = guard_bits !=
               (result_bit31 != UINT8_C(0) ? UINT64_C(0xFF)
                                           : UINT64_C(0));
    if (overflow && ovm != UINT8_C(0)) {
        shifted_raw40 = negative ? UINT64_C(0xFF80000000)
                                 : UINT64_C(0x007FFFFFFF);
    }
    if (overflow) {
        staged.payload.accumulator_write.status_delta
            .destination_overflow_action = TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1;
    }

    staged.payload.accumulator_write.value_raw40 = shifted_raw40;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_src_asm_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ld_src_asm_operands *operands)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value destination_field = {0U, 0U};

    if (instruction == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_SRC_ASM);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_LD_SRC_ASM ||
        metadata->first_word_value != UINT16_C(0xF482) ||
        metadata->first_word_mask != UINT16_C(0xFCFF) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_ASM ||
        instruction->id != TI_C54X_OPCODE_ID_LD_SRC_ASM ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFCFF)) != UINT16_C(0xF482)) {
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
                &destination_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            source_field.width_bits != UINT8_C(1) ||
            source_field.raw_value > UINT64_C(1) ||
            destination_field.width_bits != UINT8_C(1) ||
            destination_field.raw_value > UINT64_C(1)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    operands->source = (uint8_t)source_field.raw_value;
    operands->destination = (uint8_t)destination_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_src_asm_validate_inputs(
    const ti_c54x_op_input_snapshot *input, uint8_t source,
    uint8_t destination)
{
    bool source_known;
    bool destination_known;
    ti_c54x_u40_t source_raw40;

    if (input == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!input->status_sxm_known || !input->status_ovm_known ||
        !input->status_asm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_sxm > UINT8_C(1) || input->status_ovm > UINT8_C(1) ||
        input->status_asm > UINT8_C(31)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    source_known = source == UINT8_C(0) ? input->accumulator_a_known
                                        : input->accumulator_b_known;
    destination_known = destination == UINT8_C(0)
                            ? input->accumulator_a_known
                            : input->accumulator_b_known;
    if (!source_known ||
        (source != destination && !destination_known)) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    source_raw40 = source == UINT8_C(0) ? input->accumulator_a_raw40
                                        : input->accumulator_b_raw40;
    if (source_raw40 > TI_C54X_U40_MASK) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_src_asm_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands,
    ld_src_asm_operands *decoded_operands)
{
    if (operands == NULL || decoded_operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count < UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1) ||
        operands->values[1].width_bits != UINT8_C(1) ||
        operands->values[1].raw_value > UINT64_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    decoded_operands->source = (uint8_t)operands->values[0].raw_value;
    decoded_operands->destination = (uint8_t)operands->values[1].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_ld_src_asm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ld_src_asm_operands operands;
    ti_c54x_op_status status;
    uint64_t source_raw40;
    int32_t shift;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_src_asm_validate_instruction(instruction, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_src_asm_validate_inputs(input, operands.source,
                                        operands.destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    source_raw40 = operands.source == UINT8_C(0)
                       ? input->accumulator_a_raw40
                       : input->accumulator_b_raw40;
    shift = input->status_asm <= UINT8_C(15)
                ? (int32_t)input->status_asm
                : (int32_t)input->status_asm - INT32_C(32);
    return ld_src_asm_values(source_raw40, shift, input->status_sxm,
                             input->status_ovm, operands.destination,
                             effect);
}

ti_c54x_op_status ti_c54x_trusted_op_ld_src_asm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ld_src_asm_operands decoded_operands;
    ti_c54x_op_status status;
    uint64_t source_raw40;
    int32_t shift;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status =
        ld_src_asm_validate_trusted_operands(operands, &decoded_operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_src_asm_validate_inputs(input, decoded_operands.source,
                                        decoded_operands.destination);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    source_raw40 = decoded_operands.source == UINT8_C(0)
                       ? input->accumulator_a_raw40
                       : input->accumulator_b_raw40;
    shift = input->status_asm <= UINT8_C(15)
                ? (int32_t)input->status_asm
                : (int32_t)input->status_asm - INT32_C(32);
    return ld_src_asm_values(source_raw40, shift, input->status_sxm,
                             input->status_ovm,
                             decoded_operands.destination, effect);
}
