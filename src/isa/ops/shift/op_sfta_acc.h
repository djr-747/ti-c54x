#ifndef TI_C54X_ISA_OPS_SFTA_ACC_H
#define TI_C54X_ISA_OPS_SFTA_ACC_H

/* Module contract
 * Owns: pure row-less SFTA accumulator semantics over copied canonical
 * raw40 inputs, including source/destination selection, sign-extended
 * five-bit shifts, SXM-dependent right shifts, OVM saturation, and staged
 * accumulator/C/overflow effects.
 * Does not own: accumulator storage, status mutation, catalog rows, decode
 * tables, dispatch binding, sequencing, CPU lifecycle, timing,
 * repeat/interrupt policy, or diagnostics.
 */

#include "../dispatch/trusted_handler.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_sfta_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sfta_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_SFTA_ACC_H */
