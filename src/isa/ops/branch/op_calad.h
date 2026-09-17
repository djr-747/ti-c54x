#ifndef TI_C54X_ISA_OPS_BRANCH_OP_CALAD_H
#define TI_C54X_ISA_OPS_BRANCH_OP_CALAD_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for one-word CALAD
 * with the accumulator select carried in arf into a MEMORY
 * STACK_PUSH_ACCUMULATOR_DELAYED effect. Each leaf validates identity and
 * operands, gates on SP knownness and the selected accumulator
 * knownness, and stages exactly one TOS WRITE with a zero
 * return-address placeholder and all other state preserved; the
 * call target and return address resolve at the S5 seam root.
 * Does not own: catalog lookup generation, return-address
 * derivation, program-counter or stack-pointer commit, CPU
 * sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations. All inputs are borrowed read-only; outputs are written
 * only through the effect out-parameter on success.
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

ti_c54x_op_status ti_c54x_op_calad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_calad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_CALAD_H */
