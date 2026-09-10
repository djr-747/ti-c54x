/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for one-word
 * LD Xmem,SHFT,dst. Each checked leaf validates its catalog/decode
 * contract and stages an X-only dual-indirect WORD16 read; its static
 * completion validates the copied instruction and input before
 * staging the SXM-widened, SHFT-shifted accumulator write with OVM
 * saturation and sticky destination overflow. The trusted leaves
 * consume constructor-validated raw operands (raw Xar field) while
 * retaining SXM/OVM input validation.
 * Does not own: opcode inventory, generic decode, AR resolution or
 * mutation, buses, CPU/state mutation, sequencing, timing, observers,
 * allocation, logging, environment, or diagnostics. Borrowed
 * instruction/input/effect objects are used for one call only and no
 * pointer is retained; failed calls leave the caller-owned effect
 * unchanged.
 */

#include "op_ld_xmem_shift.h"

#include <stdbool.h>
#include <stddef.h>

#include "foundation/arithmetic40.h"
#include "isa/ops/dispatch/trusted_semantics.h"

typedef struct ti_c54x_ld_xmem_shift_operands {
    uint8_t destination;
    uint8_t x_ar;
    uint8_t x_mode;
    uint8_t shift;
} ti_c54x_ld_xmem_shift_operands;

static ti_c54x_op_status ld_xmem_shift_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_ld_xmem_shift_operands *operands)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value destination_field = {0U, 0U};
    ti_c54x_operand_field_value xmod_field = {0U, 0U};
    ti_c54x_operand_field_value xar_field = {0U, 0U};
    ti_c54x_operand_field_value shift_field = {0U, 0U};

    if (instruction == NULL || operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(
        TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_LD_XMEM_SHIFT ||
        metadata->first_word_mask == UINT16_C(0) ||
        (metadata->first_word_value & (uint16_t)~metadata->first_word_mask) !=
            UINT16_C(0) ||
        metadata->program_word_count == UINT8_C(0) ||
        metadata->program_word_count >
            (uint8_t)TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4 ||
        metadata->repeatability < TI_C54X_REPEATABILITY_UNRESOLVED ||
        metadata->repeatability >= TI_C54X_REPEATABILITY_COUNT ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS_STATUS_SXM_OVM ||
        instruction->id != TI_C54X_OPCODE_ID_LD_XMEM_SHIFT ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        instruction->bundle.program_word_count != metadata->program_word_count ||
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
                &layout_view, &words, UINT8_C(0), &destination_field) !=
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
            destination_field.width_bits != UINT8_C(1) ||
            destination_field.raw_value > UINT64_C(1) ||
            xmod_field.width_bits != UINT8_C(2) ||
            xmod_field.raw_value > UINT64_C(3) ||
            xar_field.width_bits != UINT8_C(2) ||
            xar_field.raw_value > UINT64_C(3) ||
            shift_field.width_bits != UINT8_C(4) ||
            shift_field.raw_value > UINT64_C(15)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    operands->destination = (uint8_t)destination_field.raw_value;
    operands->x_mode = (uint8_t)xmod_field.raw_value;
    operands->x_ar = (uint8_t)(xar_field.raw_value + UINT64_C(2));
    operands->shift = (uint8_t)shift_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_xmem_shift_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands,
    ti_c54x_ld_xmem_shift_operands *decoded_operands)
{
    if (operands == NULL || decoded_operands == NULL) {
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
    decoded_operands->destination = (uint8_t)operands->values[0].raw_value;
    decoded_operands->x_mode = (uint8_t)operands->values[1].raw_value;
    decoded_operands->x_ar =
        (uint8_t)(operands->values[2].raw_value + UINT64_C(2));
    decoded_operands->shift = (uint8_t)operands->values[3].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_xmem_shift_validate_input(
    const ti_c54x_op_input_snapshot *input)
{
    if (!input->status_sxm_known || !input->status_ovm_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    if (input->status_sxm > UINT8_C(1) || input->status_ovm > UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ld_xmem_shift_complete_values(
    uint8_t destination,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    uint8_t shift,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_op_status input_status = ld_xmem_shift_validate_input(input);
    ti_c54x_op_effect staged;
    ti_c54x_u40_t widened;
    ti_c54x_u40_t value_raw40;
    uint64_t guard;
    uint8_t bit31;
    bool overflow;
    bool negative;

    if (input_status != TI_C54X_OP_STATUS_OK) {
        return input_status;
    }

    widened = ti_c54x_u40_from_unsigned(read_value_word);
    if (input->status_sxm != UINT8_C(0) &&
        (read_value_word & UINT16_C(0x8000)) != UINT16_C(0)) {
        widened = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                  (uint64_t)read_value_word;
    }
    negative = (widened & TI_C54X_U40_SIGN_BIT) != UINT64_C(0);
    value_raw40 = ti_c54x_u40_logical_left_shift(widened, shift);
    guard = (value_raw40 >> UINT64_C(32)) & UINT64_C(0xFF);
    bit31 = (uint8_t)((value_raw40 >> UINT64_C(31)) & UINT64_C(1));
    overflow = guard != (bit31 != UINT8_C(0) ? UINT64_C(0xFF) : UINT64_C(0));
    if (overflow && input->status_ovm != UINT8_C(0)) {
        value_raw40 = negative ? UINT64_C(0xFF80000000) : UINT64_C(0x007FFFFFFF);
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = value_raw40,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .destination_overflow_action =
                    overflow ? TI_C54X_OP_DESTINATION_OVERFLOW_WRITE_1
                             : TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE}}};
    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static void ld_xmem_shift_stage_memory_read(
    const ti_c54x_ld_xmem_shift_operands *operands,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    /* Y self-aliases X: the X-only resolver reads and commits the X
     * side alone through the single-read child. */
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind =
                    TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY,
                .dual = {.x_mode = operands->x_mode,
                         .x_ar = operands->x_ar,
                         .y_mode = operands->x_mode,
                         .y_ar = operands->x_ar,
                         .reserved_word0 = UINT32_C(0),
                         .reserved_word1 = UINT32_C(0),
                         .reserved_word2 = UINT32_C(0),
                         .reserved_word3 = UINT32_C(0)}},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0),
            .complete = complete}};
    *effect = staged;
}

static ti_c54x_op_status ld_xmem_shift_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_ld_xmem_shift_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_xmem_shift_validate_instruction(instruction, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ld_xmem_shift_complete_values(operands.destination, input,
                                         read_value_word, operands.shift,
                                         next_effect);
}

ti_c54x_op_status ti_c54x_op_ld_xmem_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_xmem_shift_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_xmem_shift_validate_instruction(instruction, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_xmem_shift_validate_input(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ld_xmem_shift_stage_memory_read(&operands, ld_xmem_shift_complete,
                                    effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ld_xmem_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_xmem_shift_operands decoded_operands;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ld_xmem_shift_validate_trusted_operands(operands,
                                                     &decoded_operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ld_xmem_shift_validate_input(input);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ld_xmem_shift_stage_memory_read(&decoded_operands,
                                    ld_xmem_shift_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}
