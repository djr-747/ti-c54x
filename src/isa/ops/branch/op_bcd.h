#ifndef TI_C54X_ISA_OPS_BRANCH_OP_BCD_H
#define TI_C54X_ISA_OPS_BRANCH_OP_BCD_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for two-word BCD
 * pmad,cond with taken DELAYED_PC_WRITE and not-taken NONE
 * emission. Each leaf validates identity and operands and stages
 * no status, accumulator, T-register, or MEMORY effect.
 * Single-condition evaluation is shared with the BC leaf through
 * the branch-condition helper. The run loop validates the taken
 * absolute target, commits the sequential successor, retires, and
 * arms the CPU-owned countdown with pmad; not-taken branches
 * retire through the proven sequential bridge.
 * Does not own: catalog lookup generation, program-counter commit,
 * countdown arming, sequencing, timing, buses, memory,
 * diagnostics, or public ABI declarations. All inputs are
 * borrowed read-only; outputs are written only through the effect
 * out-parameter on success.
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

ti_c54x_op_status ti_c54x_op_bcd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_bcd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_BCD_H */
