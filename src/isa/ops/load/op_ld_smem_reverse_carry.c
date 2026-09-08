/*
 * Module contract
 *
 * Owns: checked and trusted reverse-carry MOD4/MOD7 WORD16 leaves for LD,
 * LD TS, LD shift16, and LDU accumulator/T destinations. Validation/staging
 * is delegated to the private reverse common seam; this file owns transforms.
 * Does not own: catalog/decode, address resolution, buses, CPU state,
 * sequencing, timing, observers, or diagnostics.
 */

#include "op_ld_smem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"
#include "op_ld_smem_reverse_carry_common.h"

static ti_c54x_op_status validate_sxm(const ti_c54x_op_input_snapshot *input)
{
    if (!input->status_sxm_known) return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    return input->status_sxm <= UINT8_C(1) ? TI_C54X_OP_STATUS_OK
                                          : TI_C54X_OP_STATUS_INVALID_OPERAND;
}

static ti_c54x_op_status validate_ts(const ti_c54x_op_input_snapshot *input)
{
    ti_c54x_op_status status;
    uint8_t shift;
    if (input == NULL) return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    status = validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) return status;
    if (!input->t_register_known) return TI_C54X_OP_STATUS_UNKNOWN_INPUT;
    shift = (uint8_t)(input->t_register_raw16 & UINT16_C(0x3F));
    return (shift >= UINT8_C(0x20) && shift <= UINT8_C(0x2F))
               ? TI_C54X_OP_STATUS_INVALID_OPERAND
               : TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status complete_values(
    uint8_t destination, const ti_c54x_op_input_snapshot *input,
    uint16_t read_value, uint8_t shift, bool arithmetic_right,
    ti_c54x_op_effect *next_effect)
{
    ti_c54x_u40_t value;
    ti_c54x_op_status status;

    if (input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = validate_sxm(input);
    if (status != TI_C54X_OP_STATUS_OK) return status;
    value = ti_c54x_u40_from_unsigned(read_value);
    if (input->status_sxm != 0U && (read_value & UINT16_C(0x8000)) != 0U)
        value = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) | read_value;
    value = arithmetic_right ? ti_c54x_u40_arithmetic_right_shift(value, shift)
                              : ti_c54x_u40_logical_left_shift(value, shift);
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {
            .destination = destination, .value_raw40 = value,
            .status_delta = {.c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                             .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE}}};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status complete_ld(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value,
    ti_c54x_op_effect *next_effect, ti_c54x_opcode_id id,
    ti_c54x_indirect_reverse_carry_mode mode, uint8_t shift, bool right)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_reverse_instruction(
        instruction, id, id == TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD4 ||
                              id == TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD7
                          ? TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER
                          : TI_C54X_INPUT_RESOURCES_STATUS_SXM,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) return status;
    if (id == TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD4 ||
        id == TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD7)
        return validate_ts(input) == TI_C54X_OP_STATUS_OK
                   ? complete_values(operands.destination, input, read_value,
                                     (uint8_t)((input->t_register_raw16 & 0x3FU) <= 31U
                                                   ? (input->t_register_raw16 & 0x3FU)
                                                   : 64U - (input->t_register_raw16 & 0x3FU)),
                                     (input->t_register_raw16 & 0x3FU) > 31U,
                                     next_effect)
                   : validate_ts(input);
    return complete_values(operands.destination, input, read_value, shift, right,
                           next_effect);
}

static ti_c54x_op_status complete_ldu(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value,
    ti_c54x_op_effect *next_effect, ti_c54x_opcode_id id,
    ti_c54x_indirect_reverse_carry_mode mode)
{
    ti_c54x_ld_smem_operands operands;
    ti_c54x_op_status status;

    if (instruction == NULL || input == NULL || next_effect == NULL) {
        return TI_C54X_OP_STATUS_INVALID_ARGUMENT;
    }
    status = ti_c54x_ld_smem_validate_reverse_instruction(
        instruction, id, TI_C54X_INPUT_RESOURCES_NONE,
        TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4, mode, &operands);
    if (status != TI_C54X_OP_STATUS_OK) return status;
    *next_effect = (ti_c54x_op_effect){
        .kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE,
        .payload.accumulator_write = {.destination = operands.destination,
                                       .value_raw40 = read_value,
                                       .status_delta = {
                                           .c_action = TI_C54X_OP_STATUS_BIT_PRESERVE,
                                           .tc_action = TI_C54X_OP_STATUS_BIT_PRESERVE}}};
    return TI_C54X_OP_STATUS_OK;
}

static ti_c54x_op_status complete_t(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value,
    ti_c54x_op_effect *next_effect, ti_c54x_opcode_id id,
    ti_c54x_indirect_reverse_carry_mode mode)
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
        .kind = TI_C54X_OP_EFFECT_T_REGISTER_WRITE,
        .payload.t_register_write = {.raw_value = read_value}};
    return TI_C54X_OP_STATUS_OK;
}

#define DEFINE_ACC(name, ID4, ID7, MODE4, MODE7, shift, right)                  \
    static ti_c54x_op_status name##_complete4(                                 \
        const ti_c54x_decoded_instruction *i, const ti_c54x_op_input_snapshot *in, \
        uint16_t value, ti_c54x_op_effect *next)                                \
    { return complete_ld(i, in, value, next, ID4, MODE4, shift, right); }        \
    static ti_c54x_op_status name##_complete7(                                 \
        const ti_c54x_decoded_instruction *i, const ti_c54x_op_input_snapshot *in, \
        uint16_t value, ti_c54x_op_effect *next)                                \
    { return complete_ld(i, in, value, next, ID7, MODE7, shift, right); }        \
    ti_c54x_op_status ti_c54x_op_##name##_mod4(                                \
        const ti_c54x_decoded_instruction *i, const ti_c54x_op_input_snapshot *in, \
        ti_c54x_op_effect *e)                                                    \
    { if (i == NULL || in == NULL || e == NULL) return TI_C54X_OP_STATUS_INVALID_ARGUMENT; ti_c54x_ld_smem_operands o; ti_c54x_op_status s = \
      ti_c54x_ld_smem_validate_reverse_instruction(i, ID4, RES, LAYOUT, MODE4, &o); \
      if (s != TI_C54X_OP_STATUS_OK || (s = INPUT(in)) != TI_C54X_OP_STATUS_OK) return s; \
      ti_c54x_ld_smem_stage_reverse_memory_read(&o, MODE4, name##_complete4, e); return s; } \
    ti_c54x_op_status ti_c54x_op_##name##_mod7(                                \
        const ti_c54x_decoded_instruction *i, const ti_c54x_op_input_snapshot *in, \
        ti_c54x_op_effect *e)                                                    \
    { if (i == NULL || in == NULL || e == NULL) return TI_C54X_OP_STATUS_INVALID_ARGUMENT; ti_c54x_ld_smem_operands o; ti_c54x_op_status s = \
      ti_c54x_ld_smem_validate_reverse_instruction(i, ID7, RES, LAYOUT, MODE7, &o); \
      if (s != TI_C54X_OP_STATUS_OK || (s = INPUT(in)) != TI_C54X_OP_STATUS_OK) return s; \
      ti_c54x_ld_smem_stage_reverse_memory_read(&o, MODE7, name##_complete7, e); return s; } \
    ti_c54x_op_status ti_c54x_trusted_op_##name##_mod4(                         \
        const ti_c54x_decoded_instruction *i, const ti_c54x_trusted_operand_values *v, \
        const ti_c54x_op_input_snapshot *in, ti_c54x_op_effect *e)               \
    { if (i == NULL || v == NULL || in == NULL || e == NULL) return TI_C54X_OP_STATUS_INVALID_ARGUMENT; ti_c54x_ld_smem_operands o; ti_c54x_op_status s = \
      ti_c54x_ld_smem_validate_reverse_trusted_operands(i,v,ID4,RES,LAYOUT,MODE4,&o); \
      if (s != TI_C54X_OP_STATUS_OK || (s = INPUT(in)) != TI_C54X_OP_STATUS_OK) return s; \
      ti_c54x_ld_smem_stage_reverse_memory_read(&o, MODE4, name##_complete4, e); return s; } \
    ti_c54x_op_status ti_c54x_trusted_op_##name##_mod7(                         \
        const ti_c54x_decoded_instruction *i, const ti_c54x_trusted_operand_values *v, \
        const ti_c54x_op_input_snapshot *in, ti_c54x_op_effect *e)               \
    { if (i == NULL || v == NULL || in == NULL || e == NULL) return TI_C54X_OP_STATUS_INVALID_ARGUMENT; ti_c54x_ld_smem_operands o; ti_c54x_op_status s = \
      ti_c54x_ld_smem_validate_reverse_trusted_operands(i,v,ID7,RES,LAYOUT,MODE7,&o); \
      if (s != TI_C54X_OP_STATUS_OK || (s = INPUT(in)) != TI_C54X_OP_STATUS_OK) return s; \
      ti_c54x_ld_smem_stage_reverse_memory_read(&o, MODE7, name##_complete7, e); return s; }

#define RES TI_C54X_INPUT_RESOURCES_STATUS_SXM
#define LAYOUT TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4
#define INPUT(in) validate_sxm(in)
DEFINE_ACC(ld_smem, TI_C54X_OPCODE_ID_LD_SMEM_MOD4, TI_C54X_OPCODE_ID_LD_SMEM_MOD7,
           TI_C54X_INDIRECT_REVERSE_CARRY_MOD4, TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, 0U, false)
DEFINE_ACC(ld_smem_shift16, TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD4,
           TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD7,
           TI_C54X_INDIRECT_REVERSE_CARRY_MOD4, TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, 16U, false)
#undef INPUT
#undef LAYOUT
#undef RES

#define RES TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER
#define LAYOUT TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4
#define INPUT(in) validate_ts(in)
DEFINE_ACC(ld_smem_ts, TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD4,
           TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD7,
           TI_C54X_INDIRECT_REVERSE_CARRY_MOD4, TI_C54X_INDIRECT_REVERSE_CARRY_MOD7, 0U, false)
#undef INPUT
#undef LAYOUT
#undef RES

#undef DEFINE_ACC

#define DEFINE_LDU(ID4, ID7, MODE4, MODE7)                                     \
    static ti_c54x_op_status ldu_complete4(const ti_c54x_decoded_instruction *i, \
        const ti_c54x_op_input_snapshot *in, uint16_t v, ti_c54x_op_effect *n)  \
    { return complete_ldu(i,in,v,n,ID4,MODE4); }                               \
    static ti_c54x_op_status ldu_complete7(const ti_c54x_decoded_instruction *i, \
        const ti_c54x_op_input_snapshot *in, uint16_t v, ti_c54x_op_effect *n)  \
    { return complete_ldu(i,in,v,n,ID7,MODE7); }                               \
    ti_c54x_op_status ti_c54x_op_ldu_smem_mod4(const ti_c54x_decoded_instruction *i, const ti_c54x_op_input_snapshot *in, ti_c54x_op_effect *e) { if(i==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT; ti_c54x_ld_smem_operands o; ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_instruction(i,ID4,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,MODE4,&o); if(s!=TI_C54X_OP_STATUS_OK)return s; ti_c54x_ld_smem_stage_reverse_memory_read(&o,MODE4,ldu_complete4,e); return s; } \
    ti_c54x_op_status ti_c54x_op_ldu_smem_mod7(const ti_c54x_decoded_instruction *i, const ti_c54x_op_input_snapshot *in, ti_c54x_op_effect *e) { if(i==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT; ti_c54x_ld_smem_operands o; ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_instruction(i,ID7,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,MODE7,&o); if(s!=TI_C54X_OP_STATUS_OK)return s; ti_c54x_ld_smem_stage_reverse_memory_read(&o,MODE7,ldu_complete7,e); return s; } \
    ti_c54x_op_status ti_c54x_trusted_op_ldu_smem_mod4(const ti_c54x_decoded_instruction *i, const ti_c54x_trusted_operand_values *v, const ti_c54x_op_input_snapshot *in, ti_c54x_op_effect *e) { if(i==NULL||v==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT; ti_c54x_ld_smem_operands o; ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_trusted_operands(i,v,ID4,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,MODE4,&o); if(s!=TI_C54X_OP_STATUS_OK)return s; ti_c54x_ld_smem_stage_reverse_memory_read(&o,MODE4,ldu_complete4,e); return s; } \
    ti_c54x_op_status ti_c54x_trusted_op_ldu_smem_mod7(const ti_c54x_decoded_instruction *i, const ti_c54x_trusted_operand_values *v, const ti_c54x_op_input_snapshot *in, ti_c54x_op_effect *e) { if(i==NULL||v==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT; ti_c54x_ld_smem_operands o; ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_trusted_operands(i,v,ID7,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,MODE7,&o); if(s!=TI_C54X_OP_STATUS_OK)return s; ti_c54x_ld_smem_stage_reverse_memory_read(&o,MODE7,ldu_complete7,e); return s; }

DEFINE_LDU(TI_C54X_OPCODE_ID_LDU_SMEM_MOD4, TI_C54X_OPCODE_ID_LDU_SMEM_MOD7,
           TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
           TI_C54X_INDIRECT_REVERSE_CARRY_MOD7)

#undef DEFINE_LDU

/* T-register completion has no input resource, but still validates the exact
 * decoded ID/layout/mode before it stages its transform. */
#define DEFINE_T(ID4, ID7, MODE4, MODE7)                                       \
    static ti_c54x_op_status t_complete4(const ti_c54x_decoded_instruction *i, const ti_c54x_op_input_snapshot *in, uint16_t v, ti_c54x_op_effect *n){return complete_t(i,in,v,n,ID4,MODE4);} \
    static ti_c54x_op_status t_complete7(const ti_c54x_decoded_instruction *i, const ti_c54x_op_input_snapshot *in, uint16_t v, ti_c54x_op_effect *n){return complete_t(i,in,v,n,ID7,MODE7);} \
    ti_c54x_op_status ti_c54x_op_ld_t_smem_mod4(const ti_c54x_decoded_instruction *i,const ti_c54x_op_input_snapshot *in,ti_c54x_op_effect *e){if(i==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT;ti_c54x_ld_smem_operands o;ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_instruction(i,ID4,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,MODE4,&o);if(s!=TI_C54X_OP_STATUS_OK)return s;ti_c54x_ld_smem_stage_reverse_memory_read(&o,MODE4,t_complete4,e);return s;} \
    ti_c54x_op_status ti_c54x_op_ld_t_smem_mod7(const ti_c54x_decoded_instruction *i,const ti_c54x_op_input_snapshot *in,ti_c54x_op_effect *e){if(i==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT;ti_c54x_ld_smem_operands o;ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_instruction(i,ID7,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,MODE7,&o);if(s!=TI_C54X_OP_STATUS_OK)return s;ti_c54x_ld_smem_stage_reverse_memory_read(&o,MODE7,t_complete7,e);return s;} \
    ti_c54x_op_status ti_c54x_trusted_op_ld_t_smem_mod4(const ti_c54x_decoded_instruction *i,const ti_c54x_trusted_operand_values *v,const ti_c54x_op_input_snapshot *in,ti_c54x_op_effect *e){if(i==NULL||v==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT;ti_c54x_ld_smem_operands o;ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_trusted_operands(i,v,ID4,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,MODE4,&o);if(s!=TI_C54X_OP_STATUS_OK)return s;ti_c54x_ld_smem_stage_reverse_memory_read(&o,MODE4,t_complete4,e);return s;} \
    ti_c54x_op_status ti_c54x_trusted_op_ld_t_smem_mod7(const ti_c54x_decoded_instruction *i,const ti_c54x_trusted_operand_values *v,const ti_c54x_op_input_snapshot *in,ti_c54x_op_effect *e){if(i==NULL||v==NULL||in==NULL||e==NULL)return TI_C54X_OP_STATUS_INVALID_ARGUMENT;ti_c54x_ld_smem_operands o;ti_c54x_op_status s=ti_c54x_ld_smem_validate_reverse_trusted_operands(i,v,ID7,TI_C54X_INPUT_RESOURCES_NONE,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,MODE7,&o);if(s!=TI_C54X_OP_STATUS_OK)return s;ti_c54x_ld_smem_stage_reverse_memory_read(&o,MODE7,t_complete7,e);return s;}

DEFINE_T(TI_C54X_OPCODE_ID_LD_T_SMEM_MOD4, TI_C54X_OPCODE_ID_LD_T_SMEM_MOD7,
         TI_C54X_INDIRECT_REVERSE_CARRY_MOD4,
         TI_C54X_INDIRECT_REVERSE_CARRY_MOD7)

#undef DEFINE_T
