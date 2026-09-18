#ifndef TI_C54X_ISA_OPS_ARITHMETIC_ABS_ACC_H
#define TI_C54X_ISA_OPS_ARITHMETIC_ABS_ACC_H

/* Module contract
 * Owns: pure checked/trusted ABS src[,dst] semantics (dst40 =
 * absolute value of src40 with 40-bit wrap) over the shared
 * ACCUMULATOR_SOURCE_DESTINATION layout, with OVM saturation of a
 * guard-overflowing magnitude to 007FFFFFFFh plus sticky OVdst,
 * C set from a zero stored result, and TC preserved. The
 * single-word opcode (0xF485 | S<<9 | D<<8, low byte fixed 0x85)
 * is validated directly against local constants with the UNKNOWN
 * catalog identity (rows land later; the maintainer flips this to
 * the real id at merge). An explicit dst selects the write
 * target; an omitted dst is assembled with D == S so the absolute
 * value returns to the source itself.
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

ti_c54x_op_status ti_c54x_op_abs_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_abs_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_ABS_ACC_H */
