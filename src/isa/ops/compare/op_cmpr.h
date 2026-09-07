#ifndef TI_C54X_ISA_OPS_COMPARE_CMPR_H
#define TI_C54X_ISA_OPS_COMPARE_CMPR_H

/* Module contract
 * Owns: checked CMPR CC,ARx semantic declaration for the pure unsigned
 * auxiliary-register comparison.  The handler stages one copied TC status-bit
 * effect and never mutates architectural state.
 * Does not own: catalog/decode production, auxiliary-register collection,
 * status mutation, sequencing, CPU lifecycle, memory, timing, or diagnostics.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_cmpr(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_COMPARE_CMPR_H */
