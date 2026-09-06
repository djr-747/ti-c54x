/* Module contract
 * Owns: pure OR two-word immediate accumulator semantics as the family leaf
 * over the shared logical-immediate transaction.
 * Does not own: shared operand validation or computation, catalog inventory,
 * generated lookup, decode tables, accumulator storage, status mutation,
 * sequencing, CPU lifecycle, timing, memory, repeat/interrupt policy,
 * diagnostics, or public API.
 */

#ifndef TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_OR_H
#define TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_OR_H

#include "isa/decode/opcode_decode.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_or_immediate(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_or_immediate_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_or_immediate_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_OR_H */
