/*
 * Module contract
 *
 * Owns: the RPTB catalog identity and the checked and
 * trusted entry points into the shared block-start
 * core.
 * Does not own: catalog rows, dispatch bindings, operand
 * layouts, BRC/RSA/REA state, CPU/state mutation,
 * sequencing, timing, observers, diagnostics, allocation,
 * logging, or environment.
 */

#ifndef TI_C54X_ISA_OPS_REPEAT_OP_RPTB_H
#define TI_C54X_ISA_OPS_REPEAT_OP_RPTB_H

#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Checked leaf: validates the decoded instruction and stages the
 * block-repeat-start effect carrying the copied pmad. Borrowed
 * instruction, input, and effect objects are used for one call
 * only; no pointer is retained and failed calls leave the caller
 * effect unchanged. */
ti_c54x_op_status ti_c54x_op_rptb(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

/* Trusted leaf: takes one operand with exact width (16)
 * already extracted through the branch-pmad layout. Borrowed
 * operands, input, and effect objects are used for one call
 * only; no pointer is retained and failed calls leave the
 * caller effect unchanged. */
ti_c54x_op_status ti_c54x_trusted_op_rptb(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_REPEAT_OP_RPTB_H */
