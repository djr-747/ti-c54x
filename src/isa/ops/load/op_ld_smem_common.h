#ifndef TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_COMMON_H
#define TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_COMMON_H

/*
 * Module contract
 *
 * Owns: private shared validation and MEMORY-effect staging for the neutral
 * standard-indirect accumulator loads. It extracts the copied destination,
 * ARF, and MOD operands from a catalog/decode contract and stages a copied
 * single-indirect WORD16 read after rejecting MOD3 loads. It retains no
 * caller-owned pointer and leaves caller effects unchanged on failure.
 * Does not own: opcode-family completion transforms, SXM/T input semantics,
 * decode tables, address resolution, buses, CPU state, sequencing, timing,
 * observers, allocation, logging, environment, or diagnostics.
 */

#include <stdint.h>

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

void ti_c54x_ld_smem_stage_memory_read(
    const ti_c54x_ld_smem_operands *operands,
    ti_c54x_op_memory_complete_fn complete,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_COMMON_H */
