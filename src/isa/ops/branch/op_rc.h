#ifndef TI_C54X_ISA_OPS_BRANCH_OP_RC_H
#define TI_C54X_ISA_OPS_BRANCH_OP_RC_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for one-word
 * RC cond with taken RET-shaped MEMORY stack-pop emission
 * versus not-taken NONE. Only the sixteen image-evidenced
 * single conditions evaluate through the shared
 * branch-condition helper; every other condition rejects as
 * deferred. Taken returns additionally gate known SP per the
 * RET leaf convention; not-taken returns consume condition
 * inputs only. Each leaf validates identity and operands and
 * stages no status, accumulator, T-register, or non-stack
 * MEMORY effect. The shadow word 0xFC00 decodes as RET
 * upstream, so no leaf path observes it.
 * The root transaction pops TOS, commits SP+1, and redirects
 * PC to the popped value for taken returns, and advances
 * sequentially for not-taken returns.
 * Does not own: catalog lookup generation, stack reads,
 * countdown arming, sequencing, timing, buses, memory,
 * diagnostics, or public ABI declarations. All inputs are
 * borrowed read-only; outputs are written only through the
 * effect out-parameter on success.
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

ti_c54x_op_status ti_c54x_op_rc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_rc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_RC_H */
