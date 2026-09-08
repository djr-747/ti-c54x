#ifndef TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_REVERSE_CARRY_COMMON_H
#define TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_REVERSE_CARRY_COMMON_H

/*
 * Module contract
 *
 * Owns: private validation and MEMORY staging shared by reverse-carry load
 * leaves. It validates copied catalog/decode and trusted operand contracts,
 * then stages a typed READ effect. Does not own: family completion transforms,
 * opcode tables, address resolution, buses, CPU state, or sequencing.
 */

#include "address/indirect_reverse_carry.h"
#include "op_ld_smem_common.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_ld_smem_validate_reverse_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_indirect_reverse_carry_mode expected_mode,
    ti_c54x_ld_smem_operands *operands);

ti_c54x_op_status ti_c54x_ld_smem_validate_reverse_trusted_operands(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_indirect_reverse_carry_mode expected_mode,
    ti_c54x_ld_smem_operands *operands);

void ti_c54x_ld_smem_stage_reverse_memory_read(
    const ti_c54x_ld_smem_operands *operands,
    ti_c54x_indirect_reverse_carry_mode mode,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif
