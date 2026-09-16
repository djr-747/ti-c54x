/*
 * Module contract
 *
 * Owns: pure checked/trusted RETF semantics staging a MEMORY
 * STACK_POP_RTN READ effect with a zero staged value and an
 * INTM-clearing completion, gated on a known stack pointer plus a
 * known RTN register. The completion ignores the frozen RTN value
 * and stages a STATUS_BIT_WRITE clearing ST1 bit 11; the root
 * freezes RTN at prepare and commits the completion through the
 * generic pop-completion seam before the pop architecture commit
 * with no bus traffic.
 * Does not own: catalog lookup generation, RTN value derivation,
 * completion commit, program-counter or stack-pointer commit, CPU
 * sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations.
 */

#include "op_retf.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/dispatch/trusted_semantics.h"

/* ST1 bit 11 is INTM per the ST1 register layout; RETF[D] clears
 * it to enable interrupts (SPRU172C p. 4-141). Owned here (not
 * shared with the RETE family) until RETFD lands and earns a
 * common module. */
#define TI_C54X_RETF_INTM_BIT_INDEX UINT8_C(11)

static ti_c54x_op_status retf_complete_intm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    (void)instruction;
    (void)input;
    (void)read_value_word;
    if (next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_STATUS_BIT_WRITE,
        .payload.status_bit_write = {
            .target_register = TI_C54X_OP_STATUS_REGISTER_ST1,
            .bit_index = TI_C54X_RETF_INTM_BIT_INDEX,
            .raw_value = UINT8_C(0)}};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status retf_values(
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (!input->stack_pointer_known || !input->return_register_known) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind =
                    TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16,
            },
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0),
            .rmw_operation = (uint8_t)
                TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            .complete = retf_complete_intm,
        }};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status retf_validate_instruction(
    const ti_c54x_decoded_instruction *instruction)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;

    if (instruction == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RETF);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_RETF ||
        metadata->first_word_value != UINT16_C(0xF49B) ||
        metadata->first_word_mask != UINT16_C(0xFFFF) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout != TI_C54X_OPERAND_LAYOUT_NONE ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_STACK_POINTER_RETURN_REGISTER ||
        instruction->id != TI_C54X_OPCODE_ID_RETF ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   UINT16_C(0xFFFF)) != UINT16_C(0xF49B)) {
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

static ti_c54x_op_status retf_validate_trusted_operands(
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

ti_c54x_op_status ti_c54x_op_retf(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = retf_validate_instruction(instruction);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return retf_values(input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_retf(
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
    status = retf_validate_trusted_operands(operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return retf_values(input, effect);
}
