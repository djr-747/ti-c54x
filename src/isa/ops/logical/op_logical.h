#ifndef TI_C54X_ISA_OPS_LOGICAL_H
#define TI_C54X_ISA_OPS_LOGICAL_H

/* Module contract
 * Owns: pure AND, OR, and XOR accumulator semantics over copied canonical
 * raw40 inputs, including source/destination selection and signed five-bit
 * full-width shifts.
 * Does not own: catalog inventory, decode tables, accumulator storage, status
 * mutation, sequencing, CPU lifecycle, timing, memory, or diagnostics.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_and_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_or_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_xor_acc_shift(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOGICAL_H */
