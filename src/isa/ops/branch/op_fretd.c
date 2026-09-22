/*
 * Module contract
 *
 * Owns: pure checked/trusted FRETD semantics staging a MEMORY
 * STACK_POP_FAR_DELAYED READ effect with a zero staged value and
 * null completion, gated on a known stack pointer.
 * Does not own: catalog lookup generation, TOS value derivation,
 * program-counter or stack-pointer commit, CPU sequencing, timing,
 * buses, memory, diagnostics, or public ABI declarations.
 */

#include "op_fretd.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status fretd_values(
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (!input->stack_pointer_known) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind =
                    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_FAR_DELAYED_WORD16,
            },
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0),
            .rmw_operation = (uint8_t)
                TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            .complete = NULL,
        }};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status fretd_validate_instruction(
    const ti_c54x_decoded_instruction *instruction)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;

    if (instruction == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FRETD);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_FRETD ||
        metadata->first_word_value != UINT16_C(0xF6E4) ||
        metadata->first_word_mask != UINT16_C(0xFFFF) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_NONE ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_STACK_POINTER ||
        instruction->id != TI_C54X_OPCODE_ID_FRETD ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFFFF)) != UINT16_C(0xF6E4)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    if (ti_c54x_operand_layout_set_resolve(
            metadata->operand_layout, metadata->program_word_count,
            &layout_view) != TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
        layout_view.descriptor == NULL ||
        layout_view.descriptor->operand_count != UINT8_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }

    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status fretd_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands)
{
    if (operands == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(0)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_fretd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = fretd_validate_instruction(instruction);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return fretd_values(input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_fretd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = fretd_validate_trusted_operands(operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return fretd_values(input, effect);
}
