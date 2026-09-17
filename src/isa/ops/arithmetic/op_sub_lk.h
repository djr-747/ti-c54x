#ifndef TI_C54X_ISA_OPS_ARITHMETIC_SUB_LK_H
#define TI_C54X_ISA_OPS_ARITHMETIC_SUB_LK_H

/* Module contract
 * Owns: pure checked/trusted SUB #lk semantics over the two shared
 * 2-word accumulator layouts: syn7 SUB #lk[,SHFT],src[,dst] widens
 * the 16-bit extension per SXM then shifts left by SHFT (0-15),
 * and syn8 SUB #lk,16,src[,dst] shifts the widened value left by
 * 16. Both compute src - (lk << shift) -> dst with OVM saturation
 * plus sticky OVdst. C is written from the low-32 no-borrow rule
 * (set iff no borrow); TC is preserved. No catalog rows exist for
 * these forms yet, so entry points key on the shared layouts plus
 * the SUB first-word discriminators instead of opcode metadata;
 * dispatch binding follows when the rows land.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, buses, CPU/state mutation, sequencing, timing,
 * observers, allocation, logging, environment, or diagnostics.
 * Borrowed instruction/input objects are used for one call only
 * and no pointer is retained; failed calls leave the caller-owned
 * effect unchanged.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../dispatch/trusted_handler.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_sub_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sub_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_SUB_LK_H */
