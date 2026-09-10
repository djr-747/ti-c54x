/*
 * Module contract
 *
 * Owns: pure checked/trusted CC pmad,cond semantics over the
 * sixteen image-evidenced single conditions with taken
 * CALL-shaped MEMORY stack-push emission versus
 * not-taken NONE, gated on known SP for taken branches only.
 * Does not own: catalog lookup generation, stack writes,
 * countdown arming, CPU sequencing, timing, buses, memory,
 * diagnostics, or public ABI declarations.
 */

#include "op_cc.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/operand_layout.h"
#include "isa/ops/branch/op_branch_cond_common.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#define CC_FIRST_WORD_VALUE UINT16_C(0xF900)
#define CC_FIRST_WORD_MASK UINT16_C(0xFF00)

static ti_c54x_op_status cc_extract(
    const ti_c54x_decoded_instruction *instruction, uint8_t *cond,
    uint16_t *pmad)
{
    const ti_c54x_opcode_metadata *metadata;
    ti_c54x_operand_layout_set_view layout_view;
    ti_c54x_operand_field_value cond_field = {0U, 0U};
    ti_c54x_operand_field_value pmad_field = {0U, 0U};

    if (instruction == NULL || cond == NULL || pmad == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }

    metadata = ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_CC);
    if (metadata == NULL || metadata->id != TI_C54X_OPCODE_ID_CC ||
        metadata->first_word_value != CC_FIRST_WORD_VALUE ||
        metadata->first_word_mask != CC_FIRST_WORD_MASK ||
        metadata->program_word_count != UINT8_C(2) ||
        metadata->operand_layout !=
            TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16 ||
        metadata->repeatability != TI_C54X_REPEATABILITY_INELIGIBLE ||
        metadata->input_resources !=
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC_STACK_POINTER ||
        instruction->id != TI_C54X_OPCODE_ID_CC ||
        instruction->operand_layout != metadata->operand_layout ||
        instruction->repeatability != metadata->repeatability ||
        instruction->input_resources != metadata->input_resources ||
        instruction->bundle.program_word_count != UINT8_C(2) ||
        (uint16_t)(instruction->bundle.program_words[0] &
                   CC_FIRST_WORD_MASK) != CC_FIRST_WORD_VALUE) {
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
                &layout_view, &words, UINT8_C(0), &cond_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            cond_field.width_bits != UINT8_C(8) ||
            cond_field.raw_value > UINT64_C(0xFF) ||
            ti_c54x_operand_layout_set_extract_operand(
                &layout_view, &words, UINT8_C(1), &pmad_field) !=
                TI_C54X_OPERAND_LAYOUT_STATUS_OK ||
            pmad_field.width_bits != UINT8_C(16) ||
            pmad_field.raw_value > UINT64_C(0xFFFF)) {
            return TI_C54X_OP_STATUS_INVALID_DECODE;
        }
    }

    *cond = (uint8_t)cond_field.raw_value;
    *pmad = (uint16_t)pmad_field.raw_value;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status cc_emit(
    const ti_c54x_op_input_snapshot *input, bool taken,
    ti_c54x_op_effect *effect)
{
    ti_c54x_op_effect staged;

    if (input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (!taken) {
        staged = (ti_c54x_op_effect){.kind = TI_C54X_OP_EFFECT_NONE};
        *effect = staged;
        return TI_C54X_OP_STATUS_OK;
    }
    if (!input->stack_pointer_known) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    staged = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_MEMORY,
        .payload.memory = {
            .address_spec =
                {
                    .kind =
                        TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16,
                },
            .access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            .value_word = UINT16_C(0),
            .rmw_operation =
                (uint8_t)TI_C54X_OP_MEMORY_RMW_OPERATION_NONE}};
    *effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_op_cc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t cond;
    uint16_t pmad;
    bool taken = false;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = cc_extract(instruction, &cond, &pmad);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    (void)pmad;
    status = ti_c54x_branch_cond_evaluate(cond, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return cc_emit(input, taken, effect);
}

ti_c54x_op_status ti_c54x_trusted_op_cc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    uint8_t cond;
    bool taken = false;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    if (operands->count != UINT8_C(2) ||
        operands->values[0].width_bits != UINT8_C(8) ||
        operands->values[0].raw_value > UINT64_C(0xFF) ||
        operands->values[1].width_bits != UINT8_C(16) ||
        operands->values[1].raw_value > UINT64_C(0xFFFF)) {
        return TI_C54X_OP_STATUS_INVALID_OPERAND;
    }
    cond = (uint8_t)operands->values[0].raw_value;
    status = ti_c54x_branch_cond_evaluate(cond, input, &taken);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return cc_emit(input, taken, effect);
}
