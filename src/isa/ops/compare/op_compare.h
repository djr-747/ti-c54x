#ifndef TI_C54X_ISA_OPS_COMPARE_H
#define TI_C54X_ISA_OPS_COMPARE_H

/* Module contract
 * Owns: pure MAX/MIN compare-select handler declarations over copied raw40 inputs.
 * Does not own: architectural state, sequencing, or CPU lifecycle.
 */

#include "../op_input.h"
#include "../op_effect.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_max_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_op_min_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_COMPARE_H */
