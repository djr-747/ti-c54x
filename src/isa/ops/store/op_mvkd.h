#ifndef TI_C54X_ISA_OPS_STORE_OP_MVKD_H
#define TI_C54X_ISA_OPS_STORE_OP_MVKD_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the
 * two-word absolute-to-Smem DATA copy. The checked leaves
 * validate the copied instruction and extract the Smem cookie
 * plus the 16-bit absolute dmad; the copy stages one
 * SMEM_ABSOLUTE_PAIR effect (dmad leg read first, Smem DATA
 * leg written second, zero staged value, no completion). The direct
 * leaf gates DP knownness (UNKNOWN_INPUT); the indirect leaf
 * admits MOD0--MOD3 cookies and rejects extension-requiring
 * modes (INVALID_OPERAND), leaving AR unknownness to reject
 * transactionally downstream. Borrowed instruction, operand,
 * input, and effect objects are used for one call only; no
 * pointer is retained, and failed calls leave the caller effect
 * unchanged.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution, buses, CPU or architectural state,
 * sequencing, timing, observers, diagnostics, allocation,
 * logging, environment, or callbacks.
 */

#include "../../decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_mvkd_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_mvkd_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_mvkd_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_mvkd_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STORE_OP_MVKD_H */
