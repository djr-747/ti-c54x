#ifndef TI_C54X_ISA_OPS_BRANCH_OP_FCALAD_H
#define TI_C54X_ISA_OPS_BRANCH_OP_FCALAD_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for one-word
 * FCALAD far delayed accumulator call with the S source select riding
 * inside, staged into a MEMORY STACK_PUSH_FAR_ACCUMULATOR_DELAYED
 * double-push effect. Each leaf validates identity and operands,
 * demands a known stack pointer plus a known selected
 * accumulator, and stages no status or T-register effect. The
 * root stages the live PC+3 return address plus the frozen live
 * XPC at prepare, reads the target and page from the frozen
 * input snapshot, and commits SP, both TOS words, and the far
 * target atomically.
 * Does not own: catalog lookup generation, target/page/return
 * derivation, program-counter, XPC, or stack-pointer commit,
 * sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations. All inputs are borrowed read-only; outputs are
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

ti_c54x_op_status ti_c54x_op_fcalad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_fcalad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_FCALAD_H */
