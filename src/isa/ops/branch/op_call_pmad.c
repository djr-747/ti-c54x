/*
 * Module contract
 *
 * Owns: pure checked/trusted CALL pmad semantics carrying the copied
 * 16-bit extension into a MEMORY STACK_PUSH WRITE effect with a zero
 * return-address placeholder and all other state preserved, gated on
 * a known stack pointer.
 * Does not own: catalog lookup generation, return-address derivation,
 * program-counter or stack-pointer commit, CPU sequencing, timing,
 * buses, memory, diagnostics, or public ABI declarations.
 */

#include "op_call_pmad.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

static ti_c54x_op_status call_pmad_values(uint16_t pmad,
                                          const ti_c54x_op_input_snapshot *input,
                                          ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    /* pmad rides the frozen instruction bundle to the root commit; only
     * SP knownness gates this leaf. */
    (void)pmad;
    if (!input->stack_pointer_known) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind =
                    TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16,
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

static ti_c54x_op_status call_pmad_validate_instruction(
    const ti_c54x_decoded_instruction *instruction, uint16_t *pmad)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value pmad_field = {0U, 0U};

    if (instruction == NULL || pmad == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_CALL_PMAD);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_CALL_PMAD ||
        metadata->first_word_value != UINT16_C(0xF074) ||
        metadata->first_word_mask != UINT16_C(0xFFFF) ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_STACK_POINTER ||
        instruction->id != TI_C54X_OPCODE_ID_CALL_PMAD ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFFFF)) != UINT16_C(0xF074)) {
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
                &layout_view, &words, UINT8_C(0), &pmad_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            pmad_field.width_bits != UINT8_C(16) ||
            pmad_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *pmad = (uint16_t)pmad_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status call_pmad_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands, uint16_t *pmad)
{
    if (operands == NULL || pmad == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(16) ||
        operands->values[0].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    *pmad = (uint16_t)operands->values[0].raw_value;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_call_pmad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint16_t pmad;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = call_pmad_validate_instruction(instruction, &pmad);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return call_pmad_values(pmad, input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_call_pmad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint16_t pmad;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = call_pmad_validate_trusted_operands(operands, &pmad);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return call_pmad_values(pmad, input, effect);
}
