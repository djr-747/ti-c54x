#ifndef TI_C54X_ISA_OPS_ARITHMETIC_SUB_SMEM_SHIFT_H
#define TI_C54X_ISA_OPS_ARITHMETIC_SUB_SMEM_SHIFT_H

/* Module contract
 * Owns: pure checked/trusted SUB Smem SHIFT semantics for syntax 4
 * (dst = src - (Smem << SHIFT)) over direct page-0 and
 * standard-indirect MOD0--MOD3 reads through the memory effect,
 * with SXM extension, signed 5-bit shift, OVM saturation plus
 * sticky OVdst, and C written from the bit-32 borrow (C set
 * means no borrow). Exotic indirect modes (mode4 above MOD3)
 * defer as INVALID_OPERAND.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../dispatch/trusted_handler.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_sub_smem_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sub_smem_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_SUB_SMEM_SHIFT_H */
