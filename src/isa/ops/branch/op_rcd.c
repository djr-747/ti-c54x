/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for one-word
 * RCD cond with taken RETD-shaped MEMORY delayed stack-pop emission
 * versus not-taken NONE. Only the sixteen image-evidenced
 * single conditions evaluate through the shared
 * branch-condition helper; every other condition rejects as
 * deferred. Taken returns additionally gate known SP per the
 * RET leaf convention; not-taken returns consume condition
 * inputs only. Each leaf validates identity and operands and
 * stages no status, accumulator, T-register, or non-stack
 * MEMORY effect. The shadow word 0xFE00 decodes as RETD
 * upstream, so no leaf path observes it.
 * The root transaction pops TOS, commits SP+1, and redirects
 * PC to the popped value for taken returns, and advances
 * sequentially for not-taken returns.
 * Does not own: catalog lookup generation, stack reads,
 * countdown arming, sequencing, timing, buses, memory,
 * diagnostics, or public ABI declarations. All inputs are
 * borrowed read-only; outputs are written only through the
 * effect out-parameter on success.
 */

#include "isa/ops/branch/op_rcd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/branch/op_branch_cond_common.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

static ti_c54x_op_status rc_validate_instruction(
    const ti_c54x_decoded_instruction *instruction)
{
    const ti_c54x_opcode_metadata *metadata;

    if (instruction == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RCD);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_RCD ||
        metadata->first_word_value != UINT16_C(0xFE00) ||
        metadata->first_word_mask != UINT16_C(0xFF00) ||
        metadata->program_word_count != UINT8_C(1) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_BRANCH_COND8 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC_STACK_POINTER) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if (instruction->id != TI_C54X_OPCODE_ID_RCD ||
        instruction->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_BRANCH_COND8 ||
        instruction->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        instruction->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC_STACK_POINTER ||
        instruction->bundle.program_word_count != UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    if ((uint16_t)(instruction->bundle.program_words[0] & UINT16_C(0xFF00)) !=
        UINT16_C(0xFE00)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status rc_evaluate(uint8_t cond,
                                     const ti_c54x_op_input_snapshot *input,
                                     bool *taken)
{
    ti_c54x_op_status status;

    if (input == NULL || taken == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_branch_cond_evaluate(cond, input, taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (*taken && !input->stack_pointer_known) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status rc_emit(bool taken, ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!taken) {
        staged = (ti_c54x_op_effect){.kind = TI_C54X_OP_EFFECT_NONE};
        *effect = staged;
        return TI_C54X_OP_STATUS_OK;
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec =
                {
                    .kind =
                        TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16,
                },
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
            .value_word = UINT16_C(0),
            .rmw_operation =
                (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE}};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_rcd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value cond;
    ti_c54x_program_words_view words;
    bool taken;
    ti_c54x_op_status status;

    status = rc_validate_instruction(instruction);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_BRANCH_COND8, UINT8_C(1), &view) !=
            TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
        view.descriptor == NULL ||
        view.descriptor->operand_count != UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    words = (ti_c54x_program_words_view){
        instruction->bundle.program_words,
        instruction->bundle.program_word_count};
    if (ti_c54x_operand_layout_set_extract_operand(&view, &words,
                                                   UINT8_C(0), &cond) !=
            TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
        cond.width_bits != UINT8_C(8) || cond.raw_value > UINT64_C(0xFF)) {
        return TI_C54X_OP_STATUS_INVALID_DECODE;
    }
    status = rc_evaluate((uint8_t)cond.raw_value, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return rc_emit(taken, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_rcd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint64_t cond;
    bool taken;
    ti_c54x_op_status status;

    status = rc_validate_instruction(instruction);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(1)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    cond = operands->values[0].raw_value;
    if (operands->values[0].width_bits != UINT8_C(8) ||
        cond > UINT64_C(0xFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    if ((uint8_t)cond !=
        (uint8_t)(instruction->bundle.program_words[0] & UINT16_C(0xFF))) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    status = rc_evaluate((uint8_t)cond, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return rc_emit(taken, effect);
}
