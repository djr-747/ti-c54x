#ifndef TI_C54X_ISA_OPS_ARITHMETIC_NEG_ACC_H
#define TI_C54X_ISA_OPS_ARITHMETIC_NEG_ACC_H

/* Module contract
 * Owns: pure checked/trusted NEG src[,dst] semantics (dst40 =
 * 40-bit two's complement of src40) over the shared
 * ACCUMULATOR_SOURCE_DESTINATION layout, with OVM saturation of
 * the 0x80_0000_0000 minimum edge to 0x00_7FFF_FFFF plus sticky
 * OVdst, C written (1 iff src == 0), and TC preserved. The
 * single-word opcode (0xF484 | S<<9 | D<<8, low byte fixed 0x84)
 * is validated directly against local constants with the UNKNOWN
 * catalog identity until rows land; the maintainer flips the id
 * to the real opcode id at merge. An explicit dst selects the
 * write target; an omitted dst is assembled with D == S so the
 * negated value returns to the source itself.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, accumulator/status mutation, buses, CPU/state
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

ti_c54x_op_status ti_c54x_op_neg_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_neg_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_NEG_ACC_H */
