#ifndef TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_COMMON_H
#define TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_COMMON_H

/*
 * Module contract
 *
 * Owns: private shared validation and MEMORY-effect staging for the neutral
 * standard-indirect accumulator loads. It extracts the copied destination,
 * ARF, and MOD operands from a catalog/decode contract and stages copied
 * single-indirect WORD16 reads for MOD0--MOD2, AR0-indexed WORD16 reads for
 * MOD5/MOD6, and typed circular WORD16 reads for MOD8/MOD10. It retains no
 * caller-owned pointer and leaves caller effects unchanged on failure.
 * Does not own: opcode-family completion transforms, SXM/T input semantics,
 * decode tables, address resolution, buses, CPU state, sequencing, timing,
 * observers, allocation, logging, environment, or diagnostics.
 */

#include <stdint.h>

#include "address/indirect_circular.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ti_c54x_ld_smem_operands {
    uint8_t destination;
    uint8_t arf;
    uint8_t mode;
    uint8_t shift_raw5;
} ti_c54x_ld_smem_operands;

ti_c54x_op_status ti_c54x_ld_smem_validate_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_ld_smem_operands *operands);

ti_c54x_op_status ti_c54x_ld_smem_validate_trusted_operands(
    const ti_c54x_trusted_operand_values *operands,
    ti_c54x_ld_smem_operands *decoded_operands);

ti_c54x_op_status ti_c54x_ld_smem_validate_mode(uint8_t mode);

/* Shift-form validation for the two-word D/ARF/MOD/SHIFT5 layout. In
 * addition to the shared identity checks it verifies the second-word
 * fixed pattern (mask 0xFEE0 equals 0x0C40) and extracts the raw five-bit
 * shift into operands->shift_raw5. */
ti_c54x_op_status ti_c54x_ld_smem_validate_shift_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_ld_smem_operands *operands);

ti_c54x_op_status ti_c54x_ld_smem_validate_shift_trusted_operands(
    const ti_c54x_trusted_operand_values *operands,
    ti_c54x_ld_smem_operands *decoded_operands);

ti_c54x_op_status ti_c54x_ld_smem_validate_indexed_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    uint8_t expected_mode,
    ti_c54x_ld_smem_operands *operands);

ti_c54x_op_status ti_c54x_ld_smem_validate_indexed_trusted_operands(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    uint8_t expected_mode,
    ti_c54x_ld_smem_operands *operands);

ti_c54x_op_status ti_c54x_ld_smem_validate_circular_instruction(
    const ti_c54x_decoded_instruction *instruction,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_ld_smem_operands *operands);

ti_c54x_op_status ti_c54x_ld_smem_validate_circular_trusted_operands(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *trusted_operands,
    ti_c54x_opcode_id expected_id,
    ti_c54x_input_resource_mask expected_resources,
    ti_c54x_operand_layout_id expected_layout,
    ti_c54x_indirect_circular_mode expected_mode,
    ti_c54x_ld_smem_operands *operands);

void ti_c54x_ld_smem_stage_memory_read(
    const ti_c54x_ld_smem_operands *operands,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect);

void ti_c54x_ld_smem_stage_indexed_memory_read(
    const ti_c54x_ld_smem_operands *operands,
    uint8_t indexed_mode,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect);

void ti_c54x_ld_smem_stage_circular_memory_read(
    const ti_c54x_ld_smem_operands *operands,
    ti_c54x_indirect_circular_mode mode,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_COMMON_H */
