#ifndef TI_C54X_ISA_OPS_ARITHMETIC_OP_DUAL_ARITHMETIC_H
#define TI_C54X_ISA_OPS_ARITHMETIC_OP_DUAL_ARITHMETIC_H

/* Module contract
 * Owns: checked and trusted syntax-6 dual-memory ADD/SUB semantic leaves
 * across X/Y MOD0/MOD1/MOD2 addressing.  The leaves stage one dual WORD16
 * read effect carrying the row's modes and, on pair completion, one
 * accumulator write with fixed-left-16 arithmetic, OVM saturation, sticky
 * destination overflow, and fixed-shift carry policy.
 * Does not own: catalog generation, operand layout definitions, address
 * resolution, memory/bus execution, architectural mutation, sequencing,
 * timing, observers, diagnostics, or allocation.  Borrowed instruction,
 * input, and effect objects are used for one call only.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"
#include "../dispatch/trusted_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_add_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_sub_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_add_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sub_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

/* Non-MOD0/MOD0 rows across X/Y MOD0/MOD1/MOD2/MOD3 addressing. Each checked and
 * trusted pair proves its exact decoded identity and stages its row modes. */
ti_c54x_op_status ti_c54x_op_add_dual_m0_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m0_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m1_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m1_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m1_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m2_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m2_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m2_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m0_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m0_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m1_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m1_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m1_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m2_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m2_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m2_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m0_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m0_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m1_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m1_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m2_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m2_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m3_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m3_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m3_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m3_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m3_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m3_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_add_dual_m3_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_sub_dual_m3_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m0_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m0_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m1_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m1_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m1_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m2_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m2_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m2_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m0_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m0_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m1_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m1_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m1_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m2_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m2_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m2_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m0_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m0_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m1_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m1_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m2_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m2_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m3_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m3_m0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m3_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m3_m1(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m3_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m3_m2(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_add_dual_m3_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_sub_dual_m3_m3(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_OP_DUAL_ARITHMETIC_H */
