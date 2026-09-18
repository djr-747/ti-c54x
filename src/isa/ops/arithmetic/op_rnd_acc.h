#ifndef TI_C54X_ISA_OPS_ARITHMETIC_RND_ACC_H
#define TI_C54X_ISA_OPS_ARITHMETIC_RND_ACC_H

/* Module contract
 * Owns: pure checked/trusted RND src[,dst] semantics (dst40 =
 * src40 plus 8000h with 40-bit wrap) over the shared
 * ACCUMULATOR_SOURCE_DESTINATION layout, with OVM saturation of a
 * signed-32 overflowing result plus sticky OVdst and C/TC
 * preserved. The single-word opcode (0xF49F | S<<9 | D<<8, low
 * byte fixed 0x9F) is validated directly against local constants
 * with the RND_ACC catalog identity. An explicit dst selects the
 * write target; an omitted dst is assembled with D == S so the
 * rounded value returns to the source itself.
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

ti_c54x_op_status ti_c54x_op_rnd_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_rnd_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_RND_ACC_H */
