/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for INTR K;
 * see op_intr.h.
 * Does not own: opcode inventory, operand-layout definitions,
 * SP/IPTR/IFR state, buses, CPU or architectural state,
 * sequencing, timing, observers, diagnostics, allocation,
 * logging, environment, or callbacks.
 */

#include "op_intr.h"

#include <stddef.h>
#include <stdint.h>

#include "../../catalog/opcode_catalog.h"
#include "../../decode/operand_field.h"
#include "../../decode/operand_layout.h"
#include "isa/ops/memory/memory_effect.h"
#include "../../../sequencer/interrupt/take_complete.h"

#define INTR_FIRST_WORD_VALUE UINT16_C(0xF7C0)
#define INTR_FIRST_WORD_MASK UINT16_C(0xFFE0)

static ti_c54x_op_status
intr_validate_instruction(const ti_c54x_decoded_instruction *instruction,
                          ti_c54x_operand_field_value *operand)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value field = {0U, 0U};

    if (instruction == NULL || operand == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_INTR_K);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_INTR_K ||
        (metadata->input_resources &
         (ti_c54x_input_resource_mask)~TI_C54X_INPUT_RESOURCES_KNOWN_MASK) !=
            TI_C54X_INPUT_RESOURCES_NONE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_STACK_POINTER ||
        metadata->first_word_value != INTR_FIRST_WORD_VALUE ||
        metadata->first_word_mask != INTR_FIRST_WORD_MASK ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_TRAP_IMMEDIATE_K ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        instruction->id != TI_C54X_OPCODE_ID_INTR_K ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(1) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   metadata->first_word_mask) !=
            metadata->first_word_value) {
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
                &layout_view, &words, UINT8_C(0), &field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            field.width_bits != UINT8_C(5) ||
            field.raw_value > UINT64_C(0x1F)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }
    *operand = field;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status
intr_stage(const ti_c54x_op_input_snapshot *input,
           ti_c54x_op_effect *effect)
{
    /* K rides the frozen bundle word to the root (which freezes
     * PC+1 and the K vector); the staged effect carries only the
     * all-zero intr-push arm with a zero value and the
     * INTM-setting take completion. Unknown SP rejects: the
     * root cannot plan TOS. */
    ti_c54x_op_effect staged = {
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec = {
                .kind = TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_INTR_WORD16,
                .mode = 0,
                .indexed_mode = 0,
                .circular_mode = 0,
                .reverse_carry_mode = 0,
                .arf = 0},
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .value_word = UINT16_C(0),
            .rmw_operation =
                (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE,
            .transfer_reserved = UINT8_C(0),
            .complete = ti_c54x_take_complete_intm}};

    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!input->stack_pointer_known) {
        return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    }
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_intr(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    ti_c54x_operand_field_value operand;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (intr_validate_instruction(instruction, &operand) !=
        TI_C54X_OP_STATUS_OK) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    (void)operand;
    return intr_stage(input, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_intr(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)
{
    (void)instruction;

    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(1) ||
        operands->values[0].width_bits != UINT8_C(5) ||
        operands->values[0].raw_value > UINT64_C(0x1F)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return intr_stage(input, effect);
}
