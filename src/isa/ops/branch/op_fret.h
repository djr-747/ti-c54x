#ifndef TI_C54X_ISA_OPS_BRANCH_OP_FRET_H
#define TI_C54X_ISA_OPS_BRANCH_OP_FRET_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for one-word FRET
 * with no operands staged into a MEMORY STACK_POP_FAR READ effect
 * with a zero staged value and null completion. Each leaf
 * validates identity and stages no status, accumulator, or
 * T-register effect itself. The root reads the return extension
 * page at old SP, reads the return address at old SP + 1, then
 * commits SP + 2 plus the popped PC+XPC atomically.
 * Does not own: catalog lookup generation, TOS value derivation,
 * completion commit, program-counter or stack-pointer commit,
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

ti_c54x_op_status ti_c54x_op_fret(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_fret(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_FRET_H */
