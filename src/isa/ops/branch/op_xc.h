#ifndef TI_C54X_ISA_OPS_BRANCH_OP_XC_H
#define TI_C54X_ISA_OPS_BRANCH_OP_XC_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for one-word
 * XC n,cond with taken NONE emission versus not-taken
 * CONDITIONAL_SKIP staging. The sixteen image-evidenced single
 * conditions evaluate through the shared branch-condition helper
 * and UNC takes unconditionally; BIO/NBIO, overflow, and
 * multi-condition combination fields have no input carrier and
 * reject as deferred INVALID_OPERAND. Each leaf validates
 * identity and operands and stages no status, accumulator,
 * T-register, or MEMORY effect.
 * Does not own: catalog lookup generation, program-counter commit,
 * skip application, delay-slot accounting, sequencing, timing,
 * buses, memory, diagnostics, or public ABI declarations. All
 * inputs are borrowed read-only; outputs are written only through
 * the effect out-parameter on success.
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

ti_c54x_op_status ti_c54x_op_xc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_xc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_XC_H */
