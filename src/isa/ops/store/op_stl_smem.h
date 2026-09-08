#ifndef TI_C54X_ISA_OPS_STORE_STL_SMEM_H
#define TI_C54X_ISA_OPS_STORE_STL_SMEM_H

/* Module contract
 * Owns: pure standard-indirect WORD16 STL memory-write effects over one
 * selected canonical accumulator low word, one ARF operand, and one MOD
 * operand.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_stl_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_STORE_STL_SMEM_H */
