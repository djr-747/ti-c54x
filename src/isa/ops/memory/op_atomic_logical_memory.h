#ifndef TI_C54X_ISA_OPS_MEMORY_OP_ATOMIC_LOGICAL_MEMORY_H
#define TI_C54X_ISA_OPS_MEMORY_OP_ATOMIC_LOGICAL_MEMORY_H

/*
 * Module contract
 *
 * Owns: checked and trusted semantic leaves for the two-word ANDM, ORM, and
 * XORM standard-indirect WORD16 forms plus the ADDM form, which shares
 * their ARF-extension shape but stages an ADD descriptor with OVM/SXM
 * knownness gating. Each leaf copies ARF, extension, and MOD operands
 * into the existing value-owned MEMORY carrier as a read-side transform
 * descriptor.
 * Does not own: catalog generation, decode, address resolution, buses, CPU or
 * architectural state, sequencing, timing, observers, diagnostics,
 * allocation, callbacks, or status mutation. Borrowed inputs are used for one
 * call and failed calls leave the caller-owned effect unchanged.
 */

#include "../../decode/opcode_decode.h"
#include "../dispatch/trusted_semantics.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_andm_smem_mod0_3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_orm_smem_mod0_3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_xorm_smem_mod0_3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_addm_smem_mod0_3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_andm_smem_mod0_3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_orm_smem_mod0_3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_xorm_smem_mod0_3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_addm_smem_mod0_3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_MEMORY_OP_ATOMIC_LOGICAL_MEMORY_H */
