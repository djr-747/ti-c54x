#ifndef TI_C54X_ISA_OPS_REPEAT_OP_RPT_SMEM_H
#define TI_C54X_ISA_OPS_REPEAT_OP_RPT_SMEM_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the one-word
 * `RPT Smem` repeat start. The checked leaf validates the copied
 * instruction and extracts the indirect flag plus 7-bit Smem
 * cookie; both leaves stage one DATA read through the shared
 * memory effect with a read-value completion that stages the
 * repeat start with RC = Smem through the shared immediate
 * core. Neither path inspects input values or knownness;
 * address planning owns DP/AR downstream. Borrowed
 * instruction, operand, input, and effect objects are used for
 * one call only; no pointer is retained, and failed calls leave
 * the caller effect unchanged.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, RC state, buses, CPU or
 * architectural state, sequencing, timing, observers,
 * diagnostics, allocation, logging, environment, or callbacks.
 */

#include "../../decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_rpt_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_rpt_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_REPEAT_OP_RPT_SMEM_H */
