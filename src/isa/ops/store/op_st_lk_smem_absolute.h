#ifndef TI_C54X_ISA_OPS_STORE_OP_ST_LK_SMEM_ABSOLUTE_H
#define TI_C54X_ISA_OPS_STORE_OP_ST_LK_SMEM_ABSOLUTE_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the exact
 * three-word ST #lk,*(abs) WORD16 store. The checked leaf
 * validates the exact first word locally (0x76F8), requires
 * the ST_LK_SMEM_ABSOLUTE catalog identity, and extracts the
 * indirect flag, absolute selector, 16-bit absolute DATA
 * address, and copied 16-bit immediate through the
 * INDIRECT_ADDR7_ABSOLUTE16_LK layout set; both paths share
 * one values core that stages a fire-and-forget generic
 * WORD16 memory-write effect of the copied lk value to the
 * absolute address with no MMR sidecar. Borrowed
 * instruction, operand, input, and effect objects are used
 * for one call only; no pointer is retained, and failed
 * calls leave the caller effect unchanged.
 * Does not own: opcode inventory, catalog rows, dispatch bindings,
 * operand-layout definitions, address resolution, buses, CPU or
 * architectural state, sequencing, timing, observers, diagnostics,
 * allocation, logging, environment, or callbacks.
 */

#include "../../decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_st_lk_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_st_lk_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STORE_OP_ST_LK_SMEM_ABSOLUTE_H */
