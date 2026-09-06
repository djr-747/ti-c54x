/*
 * Module contract
 *
 * Owns: the private semantic binding for the two-word immediate AND opcode
 * family. This leaf selects AND for the shared logical-immediate operation
 * builder and preserves the normal opcode-handler ABI.
 * Does not own: immediate decoding, operand-layout definitions, opcode
 * catalog rows, logical-operation arithmetic, accumulator or status storage,
 * effect commit, sequencing, timing, memory, repeat/IRQ policy, diagnostics,
 * or parent/shared build and test registration. Instruction, input, and
 * effect objects are borrowed for one call and no state is retained.
 */

#ifndef TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_AND_H
#define TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_AND_H

#include "isa/decode/opcode_decode.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_and_immediate(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_and_immediate_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_and_immediate_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_AND_H */
