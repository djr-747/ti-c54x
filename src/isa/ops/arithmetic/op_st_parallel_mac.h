/* Module contract
 * Owns: checked and trusted opcode leaves for the ST
 * parallel-MAC/MAS/MPY rows (TI SPRU172C Table 2-25), staging
 * one dual WORD16 WRITE with the leaf-frozen STH-by-ASM value
 * and the single-read completion that runs the multiply core.
 * Does not own: decode, catalog, input collection, the multiply
 * core, sequencing, buses, timing, observers, or diagnostics.
 * All pointers are borrowed for one call; the staged effect
 * carries the completion into the store-compute child root.
 */

#ifndef TI_C54X_ISA_OPS_ARITHMETIC_ST_PARALLEL_MAC_H
#define TI_C54X_ISA_OPS_ARITHMETIC_ST_PARALLEL_MAC_H

#include <stdint.h>

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../dispatch/trusted_handler.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Checked entries: the instruction must decode to the named row
 * with the exact first-word value. Trusted entries: operand
 * values carry [round, src, dst, dualxy] for MAC/MAS and
 * [src, dst, dualxy] for MPY. The input snapshot must hold both
 * accumulators, the X/Y auxiliary registers, ASM, live T, FRCT,
 * and OVM. Null arguments reject; crossed rows reject; unknown
 * inputs reject. */
ti_c54x_op_status ti_c54x_op_st_parallel_mac(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_st_parallel_mac(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_st_parallel_mas(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_st_parallel_mas(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_st_parallel_mpy(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_st_parallel_mpy(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_ST_PARALLEL_MAC_H */
