#ifndef TI_C54X_ISA_OPS_STORE_OP_ST_TRN_SMEM_DIRECT_H
#define TI_C54X_ISA_OPS_STORE_OP_ST_TRN_SMEM_DIRECT_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the row-less ST TRN
 * direct-slice WORD16 store of the copied TRN register to page-0 DATA Smem
 * (first word 0x8D00 under mask 0xFF80, I pinned 0, 7-bit direct
 * address). The checked leaf validates the catalog/decode contract and
 * extracts the I plus address fields; both paths consume only copied TRN
 * knownness/value and stage one direct memory-write effect with a null
 * completion and no delay flag. Borrowed instruction, operand, input,
 * and effect objects are used for one call only; no pointer is retained
 * and failed calls leave the caller effect unchanged.
 * Does not own: opcode inventory, operand layouts, TRN state or collection,
 * address resolution, buses, CPU/state mutation, sequencing, timing,
 * observers, diagnostics, allocation, logging, or environment.
 */

#include "../../decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_st_trn_smem_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_st_trn_smem_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STORE_OP_ST_TRN_SMEM_DIRECT_H */
