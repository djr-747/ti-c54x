/*
 * Module contract
 *
 * Owns: checked semantic execution of the one-word IDLE descriptor
 * (TI SPRU172C power-down IDLE K, K = NN+1 at bits 9-8). The leaf
 * gates K=1..3 and stages the IDLE wait effect; the reserved NN=3
 * selector rejects. A wait cannot repeat (catalog INELIGIBLE).
 * Does not own: opcode inventory, decode, program-counter sequencing,
 * CPU or status state, memory, buses, timing, diagnostics, or observer
 * callbacks. The descriptor is borrowed for the call and the
 * caller-owned effect is unchanged on every failure.
 */

#include "op_idle.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

/* Power-down modes 1-3; NN=3 is reserved. */
#define IDLE_MODE_MAX UINT8_C(3)

static ti_c54x_op_status
idle_values(uint8_t mode, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (mode < UINT8_C(1) || mode > IDLE_MODE_MAX) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }

    staged.kind = TI_C54X_OP_EFFECT_IDLE;
    staged.payload.idle.power_down_mode = mode;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status
idle_extract(const ti_c54x_decoded_instruction *instruction, uint8_t *mode)
{
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value k_field = {0U, 0U};

    if (instruction == NULL || mode == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_IDLE ||
        instruction->operand_layout != TI_C54X_OPERAND_LAYOUT_IDLE_K ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFCFF)) != UINT16_C(0xF4E1)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(TI_C54X_OPERAND_LAYOUT_IDLE_K,
                                           UINT8_C(1), &layout_view) !=
        TI_C54X_OPERAND_LAYOUT_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    {
        const ti_c54x_program_words_view words = {
            instruction->bundle.program_words,
            instruction->bundle.program_word_count};

        if (ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(0), &k_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            k_field.width_bits != UINT8_C(2)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *mode = (uint8_t)(k_field.raw_value + UINT64_C(1));
    if (*mode < UINT8_C(1) || *mode > IDLE_MODE_MAX) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status
ti_c54x_op_idle(const ti_c54x_decoded_instruction *instruction,
                const ti_c54x_op_input_snapshot *input,
                ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    uint8_t mode = UINT8_C(0);
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_IDLE);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_IDLE ||
        metadata->first_word_mask != UINT16_C(0xFCFF) ||
        instruction->id != metadata->id ||
        metadata->operand_layout <= TI_C54X_OPERAND_LAYOUT_UNKNOWN ||
        metadata->operand_layout >= TI_C54X_OPERAND_LAYOUT_COUNT ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_IDLE_K ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        metadata->program_word_count != UINT8_C(1) ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) != metadata->first_word_value) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    status = idle_extract(instruction, &mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return idle_values(mode, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_idle(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    (void)instruction;
    (void)input;

    if (operands == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(2) ||
        operands->values[0].raw_value > UINT64_C(3)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return idle_values((uint8_t)(operands->values[0].raw_value + UINT64_C(1)),
                       effect);
}
