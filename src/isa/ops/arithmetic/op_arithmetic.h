#ifndef TI_C54X_ISA_OPS_ARITHMETIC_H
#define TI_C54X_ISA_OPS_ARITHMETIC_H

/* Module contract
 * Owns: pure checked/trusted ADD_ACC_SHIFT and SUB_ACC_SHIFT semantics over
 * copied raw40 accumulators, signed five-bit shifts, SXM/OVM handling, and
 * staged accumulator/C/overflow effects.
 * Does not own: catalog lookup generation, accumulator/status mutation, CPU
 * sequencing, timing, buses, or diagnostics. Inputs and outputs are borrowed
 * for one call and effects are written only after complete validation.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../dispatch/trusted_handler.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_add_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_sub_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_add_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sub_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_H */
