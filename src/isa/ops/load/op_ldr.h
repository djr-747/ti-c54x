#ifndef TI_C54X_ISA_OPS_LOAD_OP_LDR_H
#define TI_C54X_ISA_OPS_LOAD_OP_LDR_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for one-word
 * LDR Smem,dst and the AR0-indexed MOD5/MOD6 LDR Smem,dst
 * variants, with the Smem word SXM-widened, shifted left 16,
 * and rounded by adding 0x8000 into the destination
 * accumulator. Each leaf validates identity and operands,
 * requires known SXM, preserves C/TC/OV with no OVM input, and
 * rejects the write-only MOD3 form.
 * Does not own: catalog lookup generation, AR resolution or
 * mutation, accumulator/status mutation, sequencing, timing,
 * buses, memory, diagnostics, or public ABI declarations. All
 * inputs are borrowed read-only; outputs are written only through
 * the effect out-parameter on success.
 */

#include <stdint.h>

#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_ldr(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_ldr(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_ldr_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_ldr_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_ldr_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_ldr_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOAD_OP_LDR_H */
