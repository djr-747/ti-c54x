/* Module contract
 * Owns: pure XOR immediate accumulator semantics as a thin family adapter
 * over the shared logical-immediate operation implementation.
 * Does not own: shared immediate decoding or validation, catalog metadata,
 * generated dispatch, accumulator/status storage, effect application,
 * sequencing, CPU lifecycle, memory, timing, repeat/interrupt policy, or
 * diagnostics.
 */

#ifndef TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_XOR_H
#define TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_XOR_H

#include "../../../op_effect.h"
#include "../../../op_input.h"
#include "../../../op_status.h"
#include "../../../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_xor_immediate(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_xor_immediate_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_xor_immediate_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOGICAL_IMMEDIATE_XOR_H */
