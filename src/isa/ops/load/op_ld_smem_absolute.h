/*
 * Module contract
 *
 * Owns: the LD_SMEM_ABSOLUTE catalog identity, the checked
 * and trusted entry points into the shared absolute-load
 * core, and the read-value completion that widens the
 * observed word into the selected accumulator.
 * Does not own: catalog rows, dispatch bindings, operand
 * layouts, address resolution, buses, CPU/state mutation,
 * sequencing, timing, observers, diagnostics, allocation,
 * logging, or environment.
 */

#ifndef TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_ABSOLUTE_H
#define TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_ABSOLUTE_H

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
 * absolute DATA-read effect. Borrowed instruction, input, and
 * effect objects are used for one call only; no pointer is
 * retained and failed calls leave the caller effect unchanged. */
ti_c54x_op_status ti_c54x_op_ld_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

/* Trusted leaf: takes four operands with exact widths
 * (1,1,7,16) already extracted through the absolute-load
 * layout. Borrowed operands, input, and effect objects are used
 * for one call only; no pointer is retained and failed calls
 * leave the caller effect unchanged. */
ti_c54x_op_status ti_c54x_trusted_op_ld_smem_absolute(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

/* Read-value completion: validates the copied instruction and
 * SXM knownness, then stages the accumulator write widening
 * the observed word (SXM sign extension, C/TC preserved).
 * Borrowed instruction, input, and effect objects are used for
 * one call only; no pointer is retained and failed calls leave
 * the caller effect unchanged. */
ti_c54x_op_status ti_c54x_ld_smem_absolute_complete(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, uint16_t read_value_word,
    ti_c54x_op_effect *next_effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_ABSOLUTE_H */
