#ifndef TI_C54X_ISA_OPS_ACCUMULATOR_CMPL_H
#define TI_C54X_ISA_OPS_ACCUMULATOR_CMPL_H

/* Module contract
 * Owns: pure CMPL accumulator complement semantics over one selected copied
 * raw40 input and catalog-owned source/destination operands.
 * Does not own: accumulator storage, status C mutation, decode tables,
 * sequencing, CPU lifecycle, timing, or diagnostics.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_cmpl_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_ACCUMULATOR_CMPL_H */
