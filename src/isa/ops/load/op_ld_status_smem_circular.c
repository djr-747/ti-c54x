/*
 * Module contract
 *
 * Owns: checked and trusted circular MOD8/MOD10 WORD16 load leaves for DP
 * and ASM status fields. It validates the exact catalog identity and stages
 * CIRCULAR_WORD16 reads; completion masks the copied read value.
 * Does not own: catalog/decode, address resolution or mutation, buses, CPU state,
 * sequencing, timing, observers, allocation, logging, environment, or
 * diagnostics. Borrowed objects are used for one call and no pointer kept.
 */

#include "op_ld_status_smem.h"

#include <stdint.h>

#include "address/indirect_circular.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_ld_smem_common.h"

typedef struct circular_status_descriptor {
    ti_c54x_opcode_id id;
    ti_c54x_op_status_field_target target;
    uint16_t mask;
} circular_status_descriptor;

static const circular_status_descriptor dp8 = {
    TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD8, TI_C54X_OP_STATUS_FIELD_TARGET_DP,
    UINT16_C(0x01FF)};
static const circular_status_descriptor dp10 = {
    TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD10, TI_C54X_OP_STATUS_FIELD_TARGET_DP,
    UINT16_C(0x01FF)};
static const circular_status_descriptor asm8 = {
    TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD8, TI_C54X_OP_STATUS_FIELD_TARGET_ASM,
    UINT16_C(0x001F)};
static const circular_status_descriptor asm10 = {
    TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD10, TI_C54X_OP_STATUS_FIELD_TARGET_ASM,
    UINT16_C(0x001F)};

static const circular_status_descriptor *descriptor_for_id(
    ti_c54x_opcode_id id)
{
    switch (id) {
    case TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD8:
        return &dp8;
    case TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD10:
        return &dp10;
    case TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD8:
        return &asm8;
    case TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD10:
        return &asm10;
    default:
        return 0;
    }
}

static ti_c54x_op_status circular_complete(
    ti_c54x_opcode_id id, ti_c54x_indirect_circular_mode mode,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value,
    ti_c54x_op_effect *next_effect)
{
    const circular_status_descriptor *descriptor = descriptor_for_id(id);
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (descriptor == 0 || instruction == 0 || input == 0 || next_effect == 0) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_circular_instruction(
        instruction, id, TI_C54X_INPUT_RESOURCES_NONE,
        TI_C54X_OPERAND_LAYOUT_ARF_MOD4, mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE,
        .payload.status_field_write = {
            .target = descriptor->target,
            .raw_value = (uint16_t)(read_value & descriptor->mask)}};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status checked_circular(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_opcode_id id,
    ti_c54x_indirect_circular_mode mode,
    ti_c54x_op_memory_complete_fn complete, ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == 0 || input == 0 || effect == 0) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_circular_instruction(
        instruction, id, TI_C54X_INPUT_RESOURCES_NONE,
        TI_C54X_OPERAND_LAYOUT_ARF_MOD4, mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_circular_memory_read(&operands, mode, complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status trusted_circular(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_opcode_id id,
    ti_c54x_indirect_circular_mode mode,
    ti_c54x_op_memory_complete_fn complete, ti_c54x_op_effect *effect)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == 0 || trusted_operands == 0 || input == 0 || effect == 0) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_circular_trusted_operands(
        instruction, trusted_operands, id, TI_C54X_INPUT_RESOURCES_NONE,
        TI_C54X_OPERAND_LAYOUT_ARF_MOD4, mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) {
        return status;
    }
    ti_c54x_ld_smem_stage_circular_memory_read(&operands, mode, complete, effect);
    return TI_C54X_OP_STATUS_OK;
}

#define DEFINE_CIRCULAR(suffix, id, mode)                                       \
    static ti_c54x_op_status circular_##suffix##_complete(                      \
        const ti_c54x_decoded_instruction *instruction,                         \
        const ti_c54x_op_input_snapshot *input, uint16_t read_value,             \
        ti_c54x_op_effect *next_effect)                                          \
    {                                                                             \
        return circular_complete(id, mode, instruction, input, read_value,       \
                                 next_effect);                                    \
    }                                                                             \
    ti_c54x_op_status ti_c54x_op_ld_##suffix(                                    \
        const ti_c54x_decoded_instruction *instruction,                          \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)       \
    {                                                                             \
        return checked_circular(instruction, input, id, mode,                    \
                                circular_##suffix##_complete, effect);            \
    }                                                                             \
    ti_c54x_op_status ti_c54x_trusted_op_ld_##suffix(                            \
        const ti_c54x_decoded_instruction *instruction,                          \
        const ti_c54x_trusted_operand_values *operands,                          \
        const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect)       \
    {                                                                             \
        return trusted_circular(instruction, operands, input, id, mode,          \
                                circular_##suffix##_complete, effect);            \
    }

DEFINE_CIRCULAR(dp_smem_mod8, TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD8,
                TI_C54X_INDIRECT_CIRCULAR_MOD8)
DEFINE_CIRCULAR(dp_smem_mod10, TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD10,
                TI_C54X_INDIRECT_CIRCULAR_MOD10)
DEFINE_CIRCULAR(asm_smem_mod8, TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD8,
                TI_C54X_INDIRECT_CIRCULAR_MOD8)
DEFINE_CIRCULAR(asm_smem_mod10, TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD10,
                TI_C54X_INDIRECT_CIRCULAR_MOD10)

#undef DEFINE_CIRCULAR
