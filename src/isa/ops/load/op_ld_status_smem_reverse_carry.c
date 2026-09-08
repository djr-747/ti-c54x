/*
 * Module contract
 *
 * Owns: checked and trusted reverse-carry MOD4/MOD7 WORD16 leaves for LD DP
 * and LD ASM status-field loads. The reverse common seam validates/stages
 * reads; this file owns status masks and completion transforms.
 * Does not own: catalog/decode, address resolution, buses, CPU state,
 * sequencing, timing, observers, or diagnostics.
 */

#include "op_ld_status_smem.h"

#include <stddef.h>
#include <stdint.h>

#include "op_ld_smem_reverse_carry_common.h"

static ti_c54x_op_status complete_status(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value,
    ti_c54x_op_effect *next_effect, ti_c54x_opcode_id id,
    ti_c54x_indirect_reverse_carry_mode mode,
    ti_c54x_op_status_field_target target, uint16_t mask)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_reverse_instruction(
        instruction, id, TI_C54X_INPUT_RESOURCES_NONE,
        TI_C54X_OPERAND_LAYOUT_ARF_MOD4, mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) return status;
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_STATUS_FIELD_WRITE,
        .payload.status_field_write = {.target = target,
                                       .raw_value = (uint16_t)(read_value & mask)}};
    return TI_C54X_OP_STATUS_OK;
}

#define DEFINE_STATUS(name, ID4, ID7, target, mask)                            \
    static ti_c54x_op_status name##_complete4(                                 \
        const ti_c54x_decoded_instruction *i, const ti_c54x_op_input_snapshot *in, \
        uint16_t v, ti_c54x_op_effect *n)                                       \
    { return complete_status(i,in,v,n,ID4,TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,target,mask); } \
    static ti_c54x_op_status name##_complete7(                                 \
        const ti_c54x_decoded_instruction *i, const ti_c54x_op_input_snapshot *in, \
        uint16_t v, ti_c54x_op_effect *n)                                       \
    { return complete_status(i,in,v,n,ID7,TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,target,mask); } \
    ti_c54x_op_status ti_c54x_op_##name##_mod4(const ti_c54x_decoded_instruction *i,const ti_c54x_op_input_snapshot *in,ti_c54x_op_effect *e){if(i==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT;ti_c54x_ld_smem_operands o;ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_instruction(i,ID4,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,&o);if(s!=TI_C54X_OP_STATUS_OK)return s;ti_c54x_ld_smem_stage_reverse_memory_read(&o,TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,name##_complete4,e);return s;} \
    ti_c54x_op_status ti_c54x_op_##name##_mod7(const ti_c54x_decoded_instruction *i,const ti_c54x_op_input_snapshot *in,ti_c54x_op_effect *e){if(i==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT;ti_c54x_ld_smem_operands o;ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_instruction(i,ID7,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,&o);if(s!=TI_C54X_OP_STATUS_OK)return s;ti_c54x_ld_smem_stage_reverse_memory_read(&o,TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,name##_complete7,e);return s;} \
    ti_c54x_op_status ti_c54x_trusted_op_##name##_mod4(const ti_c54x_decoded_instruction *i,const ti_c54x_trusted_operand_values *v,const ti_c54x_op_input_snapshot *in,ti_c54x_op_effect *e){if(i==NULL||v==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT;ti_c54x_ld_smem_operands o;ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_trusted_operands(i,v,ID4,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,&o);if(s!=TI_C54X_OP_STATUS_OK)return s;ti_c54x_ld_smem_stage_reverse_memory_read(&o,TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,name##_complete4,e);return s;} \
    ti_c54x_op_status ti_c54x_trusted_op_##name##_mod7(const ti_c54x_decoded_instruction *i,const ti_c54x_trusted_operand_values *v,const ti_c54x_op_input_snapshot *in,ti_c54x_op_effect *e){if(i==NULL||v==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT;ti_c54x_ld_smem_operands o;ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_trusted_operands(i,v,ID7,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,&o);if(s!=TI_C54X_OP_STATUS_OK)return s;ti_c54x_ld_smem_stage_reverse_memory_read(&o,TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,name##_complete7,e);return s;}

DEFINE_STATUS(ld_dp_smem, TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD4,
              TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD7,
              TI_C54X_OP_STATUS_FIELD_TARGET_DP, UINT16_C(0x01FF))
DEFINE_STATUS(ld_asm_smem, TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD4,
              TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD7,
              TI_C54X_OP_STATUS_FIELD_TARGET_ASM, UINT16_C(0x001F))

#undef DEFINE_STATUS
