/*
 * Module contract
 *
 * Owns: pure checked/trusted CALA semantics carrying the accumulator
 * select in arf into a MEMORY STACK_PUSH_ACCUMULATOR WRITE effect
 * with a zero return-address placeholder and all other state
 * preserved, gated on a known stack pointer and known selected
 * accumulator.
 * Does not own: catalog lookup generation, return-address derivation,
 * program-counter or stack-pointer commit, CPU sequencing, timing,
 * buses, memory, diagnostics, or public ABI declarations.
 */

#include "op_cala.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status cala_values(uint8_t select,
                                     const ti_c54x_op_input_snapshot *input,
                                     ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    /* The select rides arf to the S5 seam root, which derives the
     * call target from the frozen input snapshot; only SP and
     * selected-accumulator knownness gate this leaf. */
    if (!input->stack_pointer_known) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if (select == UINT8_C(0)) {
        if (!input->accumulator_a_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
    } else {
        if (!input->accumulator_b_known) {
            return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
        }
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind =
                    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16,
                .arf = select,
            },
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .value_word = UINT16_C(0),
            .rmw_operation = (uint8_t)
                TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            .complete = NULL,
        }};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status cala_validate_instruction(
    const ti_c54x_decoded_instruction *instruction, uint8_t *select)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value select_field = {0U, 0U};

    if (instruction == NULL || select == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_CALA);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_CALA ||
        metadata->first_word_value != UINT16_C(0xF4E3) ||
        metadata->first_word_mask != UINT16_C(0xFEFF) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STACK_POINTER ||
        instruction->id != TI_C54X_OPCODE_ID_CALA ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFEFF)) != UINT16_C(0xF4E3)) {
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
            select_field.width_bits != UINT8_C(1) ||
            select_field.raw_value > UINT64_C(1)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *select = (uint8_t)select_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status cala_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands, uint8_t *select)
{
    if (operands == NULL || select == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(1) ||
        operands->values[0].raw_value > UINT64_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *select = (uint8_t)operands->values[0].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_cala(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t select;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = cala_validate_instruction(instruction, &select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return cala_values(select, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_cala(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t select;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = cala_validate_trusted_operands(operands, &select);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return cala_values(select, input, effect);
}
