#ifndef TI_C54X_ISA_OPS_BRANCH_OP_CC_H
#define TI_C54X_ISA_OPS_BRANCH_OP_CC_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for two-word
 * CC pmad,cond with taken CALL-shaped MEMORY stack-push
 * emission versus not-taken NONE. Only the sixteen
 * image-evidenced single conditions evaluate through the shared
 * branch-condition helper; every other condition rejects as
 * deferred. Taken branches additionally gate known SP per the
 * CALL leaf convention; not-taken branches consume condition
 * inputs only. Each leaf validates identity and operands and
 * stages no status, accumulator, T-register, or non-stack
 * MEMORY effect.
 * The root transaction pushes PC+2, plans the
 * absolute pmad target, and retires through the CALL route for
 * taken branches.
 * Does not own: catalog lookup generation, stack writes,
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

ti_c54x_op_status ti_c54x_op_cc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_cc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_CC_H */
