#ifndef TI_C54X_ISA_OPS_BRANCH_OP_CALLD_H
#define TI_C54X_ISA_OPS_BRANCH_OP_CALLD_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for two-word CALLD
 * with the copied 16-bit extension and a zero return-address
 * placeholder staged into a MEMORY STACK_PUSH_DELAYED WRITE
 * effect. Each leaf validates identity and operands, demands a
 * known stack pointer, and stages no status, accumulator, or
 * T-register effect. The W67A root stages the live PC+4 return
 * address at prepare, commits SP and TOS, advances PC to the
 * sequential successor, and reports pmad deferred; the W67B run
 * loop adopts that report into the CPU-owned countdown unchanged.
 * Does not own: catalog lookup generation, return-address derivation,
 * program-counter or stack-pointer commit, sequencing, timing, buses,
 * memory, diagnostics, or public ABI declarations. All inputs are
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

ti_c54x_op_status ti_c54x_op_calld(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_calld(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_CALLD_H */
