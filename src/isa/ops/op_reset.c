/*
 * Module contract
 *
 * Owns: checked semantic execution of the exact RESET descriptor
 * (TI SPRU172C software RESET, pp. 4-137-4-138, 1 word). The leaf
 * gates the 0xF7E0 first word with no operands and stages one
 * RESET effect; the reset field list, IFR clear, and IPTR-derived
 * program-counter commit belong to the reset applier. Software
 * reset cannot repeat (catalog INELIGIBLE).
 * Does not own: opcode inventory, decode, program-counter sequencing,
 * CPU or status state, memory, buses, timing, diagnostics, or observer
 * callbacks. The descriptor is borrowed for the call and the
 * caller-owned effect is unchanged on every failure.
 */

#include "op_reset.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status
reset_values(ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged = {0};

    if (effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    staged.kind = TI_C54X_OP_EFFECT_RESET;
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status
reset_extract(const ti_c54x_decoded_instruction *instruction)
{
    if (instruction == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_RESET ||
        instruction->operand_layout != TI_C54X_OPERAND_LAYOUT_NONE ||
        instruction->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        instruction->bundle.program_words[0] != UINT16_C(0xF7E0)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status
ti_c54x_op_reset(const ti_c54x_decoded_instruction *instruction,
                 const ti_c54x_op_input_snapshot *input,
                 ti_c54x_op_effect *effect)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RESET);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_RESET ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_NONE ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources != TI_C54X_INPUT_RESOURCES_NONE ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        metadata->program_word_count != UINT8_C(1) ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        instruction->bundle.program_word_count !=
            metadata->program_word_count ||
        instruction->bundle.program_words[0] != UINT16_C(0xF7E0)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    status = reset_extract(instruction);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return reset_values(effect);
}

ti_c54x_op_status ti_c54x_trusted_op_reset(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;
    (void)input;

    if (operands == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return reset_values(effect);
}
