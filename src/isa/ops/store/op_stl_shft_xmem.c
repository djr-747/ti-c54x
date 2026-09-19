/* Module contract
 * Owns: pure Xmem WORD16 STL-by-SHFT memory-write effects over one
 * left-shifted accumulator low word (bits 15-0 of the accumulator
 * shifted logically left by SHFT), one Xmod operand, and one Xar
 * operand. SXM is provably not an input: the stored low 16 bits of
 * a left shift are pure source bits, so SXM rides hostile and
 * unconsumed.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "op_stl_shft_xmem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

/* Shift the accumulator logically left by SHFT and extract bits 15-0.
 * SHFT is the unsigned 4-bit field, so the shift is a left shift of
 * 0 to 15. */
static uint16_t stl_shft_shifted_low(uint64_t source_raw40, uint8_t shift)
{
    ti_c54x_u40_t shifted =
        ti_c54x_u40_logical_left_shift(source_raw40, (uint64_t)shift);
    return (uint16_t)(shifted & UINT64_C(0xFFFF));
}

static void stl_shft_memory_write(uint8_t x_mode, uint8_t x_ar,
                                  uint16_t value,
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
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .value_word = value,
            .complete = NULL}};
}

static ti_c54x_op_status stl_shft_values(
    uint8_t source, uint8_t x_mode, uint8_t x_ar, uint8_t shift,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_u40_t source_raw40;
    bool source_known;
    ti_c54x_op_effect staged = {0};

    if (source > UINT8_C(1) || x_mode > UINT8_C(3) ||
        x_ar < UINT8_C(2) || x_ar > UINT8_C(5) || shift > UINT8_C(15)) {
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
    stl_shft_memory_write(x_mode, x_ar,
                          stl_shft_shifted_low(source_raw40, shift),
                          &staged);
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_stl_shft_xmem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value source_field = {0U, 0U};
    ti_c54x_operand_field_value xmod_field = {0U, 0U};
    ti_c54x_operand_field_value xar_field = {0U, 0U};
    ti_c54x_operand_field_value shift_field = {0U, 0U};

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_STL_SHFT_XMEM);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_STL_SHFT_XMEM ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value &
         (uint16_t)~metadata->first_word_mask) != UINT16_C(0) ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_UNRESOLVED ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS ||
        instruction->id != TI_C54X_OPCODE_ID_STL_SHFT_XMEM ||
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
                &layout_view, &words, UINT8_C(3), &shift_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            source_field.width_bits != UINT8_C(1) ||
            xmod_field.width_bits != UINT8_C(2) ||
            xar_field.width_bits != UINT8_C(2) ||
            shift_field.width_bits != UINT8_C(4) ||
            source_field.raw_value > UINT64_C(1) ||
            xmod_field.raw_value > UINT64_C(3) ||
            xar_field.raw_value > UINT64_C(3) ||
            shift_field.raw_value > UINT64_C(15)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    return stl_shft_values((uint8_t)source_field.raw_value,
                           (uint8_t)xmod_field.raw_value,
                           (uint8_t)(xar_field.raw_value + UINT64_C(2)),
                           (uint8_t)shift_field.raw_value, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_stl_shft_xmem(
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
    return stl_shft_values((uint8_t)operands->values[0].raw_value,
                           (uint8_t)operands->values[1].raw_value,
                           (uint8_t)(operands->values[2].raw_value +
                                     UINT64_C(2)),
                           (uint8_t)operands->values[3].raw_value, input,
                           effect);
}
