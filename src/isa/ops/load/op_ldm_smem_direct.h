#ifndef TI_C54X_ISA_OPS_LOAD_OP_LDM_SMEM_DIRECT_H
#define TI_C54X_ISA_OPS_LOAD_OP_LDM_SMEM_DIRECT_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the row-less LDM
 * MMR,dst direct-slice WORD16 load of page-0 DATA MMR into accumulator
 * A or B (first word 0x4800 under mask 0xFE80, D at bit 8 selects A/B,
 * I pinned 0, 7-bit direct address). The checked leaf validates the
 * catalog/decode contract and extracts the select, I, plus address
 * fields; both paths consume no architectural inputs and stage one
 * direct memory-read effect whose static completion zero-extends the
 * read WORD16 into a canonical 40-bit accumulator write. Borrowed
 * instruction, operand, input, and effect objects are used for one call
 * only; no pointer is retained and failed calls leave the caller effect
 * unchanged.
 * Does not own: opcode inventory, operand layouts, accumulator state
 * or collection, address resolution, buses, CPU/state mutation,
 * sequencing, timing, observers, diagnostics, allocation, logging, or
 * environment.
 */

#include "../../decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_ldm_smem_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_ldm_smem_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOAD_OP_LDM_SMEM_DIRECT_H */
