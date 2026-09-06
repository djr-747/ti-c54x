#ifndef TI_C54X_ISA_OPS_ROTATE_ROLTC_H
#define TI_C54X_ISA_OPS_ROTATE_ROLTC_H

/* Module contract
 * Owns: pure ROLTC accumulator semantics over copied canonical raw40 inputs,
 * including generic source-field extraction, low32 rotate-through-TC, and
 * staged accumulator/C effects.
 * Does not own: accumulator storage, status mutation, decode tables,
 * sequencing, CPU lifecycle, timing, repeat/interrupt policy, or diagnostics.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_roltc_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_ROTATE_ROLTC_H */
