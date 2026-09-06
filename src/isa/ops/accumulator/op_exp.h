#ifndef TI_C54X_ISA_OPS_ACCUMULATOR_EXP_H
#define TI_C54X_ISA_OPS_ACCUMULATOR_EXP_H

/* Module contract
 * Owns: pure EXP accumulator semantics over one selected copied canonical
 * raw40 input and the value-only T-register effect it produces.
 * Does not own: accumulator or T-register storage, operand-layout definitions,
 * decode tables, effect application, sequencing, CPU lifecycle, timing,
 * repeat/interrupt policy, or diagnostics.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_exp_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_ACCUMULATOR_EXP_H */
