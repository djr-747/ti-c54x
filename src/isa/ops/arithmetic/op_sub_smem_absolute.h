/*
 * Module contract
 *
 * Owns: the SUB_SMEM_ABSOLUTE catalog identity, the
 * checked and trusted entry points into the shared
 * absolute-SUB core, and the single-word completion
 * that subtracts the SXM-widened observed word from
 * the selected accumulator with OVM saturation.
 * Does not own: catalog rows, dispatch bindings, operand
 * layouts, address resolution, buses, CPU/state mutation,
 * sequencing, timing, observers, diagnostics, allocation,
 * logging, or environment.
 */

#ifndef TI_C54X_ISA_OPS_ARITHMETIC_OP_SUB_SMEM_ABSOLUTE_H
#define TI_C54X_ISA_OPS_ARITHMETIC_OP_SUB_SMEM_ABSOLUTE_H

#include <stdint.h>

#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Checked leaf: validates the decoded instruction and stages the
 * absolute WORD16-read effect. Borrowed instruction, input, and
 * effect objects are used for one call only; no pointer is
 * retained and failed calls leave the caller effect unchanged. */
ti_c54x_op_status ti_c54x_op_sub_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

/* Trusted leaf: takes four operands with exact widths
 * (1,1,7,16) already extracted through the absolute
 * layout. Borrowed operands, input, and effect objects are used
 * for one call only; no pointer is retained and failed calls
 * leave the caller effect unchanged. */
ti_c54x_op_status ti_c54x_trusted_op_sub_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

/* Read completion: validates the copied instruction and
 * source/SXM/OVM knownness, then stages the accumulator
 * write subtracting the SXM-widened observed word from
 * src (OVM saturation on signed-32 overflow, C from the
 * bit-32 borrow, TC preserved, OV on overflow).
 * Borrowed instruction, input, and effect objects are used
 * for one call only; no pointer is retained and failed
 * calls leave the caller effect unchanged. */
ti_c54x_op_status ti_c54x_sub_smem_absolute_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_OP_SUB_SMEM_ABSOLUTE_H */
