/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for standard-indirect
 * WORD16 unsigned loads into accumulator A or B. The checked leaf validates
 * the catalog/decode contract, consumes the decoded MOD0--MOD2 operand, and
 * stages a generic memory-read effect; its static completion zero-extends the
 * copied WORD16 into a canonical 40-bit accumulator write. The trusted leaf
 * consumes constructor-validated operands. Neither path observes or
 * interprets SXM. Does not own: opcode
 * inventory, generic decode, AR resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging, environment,
 * or diagnostics. Borrowed instruction/input/effect objects are used for one
 * call only and no pointer is retained; failed calls leave the caller-owned
 * effect unchanged.
 */

#include "op_ldu_smem.h"

#include <stddef.h>

#include "foundation/arithmetic40.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_ld_smem_common.h"

static ti_c54x_op_status ldu_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

static ti_c54x_op_status ldu_smem_complete_values(
    uint8_t destination,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    const ti_c54x_op_effect staged = {
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination,
            .value_raw40 = (ti_c54x_u40_t)read_value_word,
            .status_delta = {
                .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE}}};

    *next_effect = staged;
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status ldu_smem_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    uint16_t read_value_word,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_instruction(
        instruction, TI_C54X_OPCODE_ID_LDU_SMEM,
        TI_C54X_INPUT_RESOURCES_NONE, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    return ldu_smem_complete_values(operands.destination, read_value_word,
                                    next_effect);
}

ti_c54x_op_status ti_c54x_op_ldu_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_instruction(
        instruction, TI_C54X_OPCODE_ID_LDU_SMEM,
        TI_C54X_INPUT_RESOURCES_NONE, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_memory_read(&operands, ldu_smem_complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

ti_c54x_op_status ti_c54x_trusted_op_ldu_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands decoded_operands;
    ti_c54x_op_status status;

    (void)instruction;
    if (operands == NULL || input == NULL || effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_trusted_operands(operands,
                                                       &decoded_operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    status = ti_c54x_ld_smem_validate_mode(decoded_operands.mode);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_memory_read(&decoded_operands, ldu_smem_complete,
                                      effect);
    return TI_C54X_OP_STATUS_OK;
}
