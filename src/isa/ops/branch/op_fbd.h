#ifndef TI_C54X_ISA_OPS_BRANCH_OP_FBD_H
#define TI_C54X_ISA_OPS_BRANCH_OP_FBD_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for two-word FBD
 * far branch-delayed with the 7-bit first-word page plus the copied 16-bit
 * extension carried verbatim into a DELAYED_FAR_PC_WRITE effect. Each leaf
 * validates identity and operands and stages exactly one far
 * program-counter target with no status, accumulator, T-register,
 * or memory effect; input state is never read.
 * Does not own: catalog lookup generation, program-counter or XPC
 * commit, sequencing, timing, buses, memory, diagnostics, or public
 * ABI declarations. All inputs are borrowed read-only; outputs are
 * written only through the effect out-parameter on success.
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

ti_c54x_op_status ti_c54x_op_fbd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_fbd(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_FBD_H */
