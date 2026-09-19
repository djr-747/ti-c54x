#ifndef TI_C54X_ISA_OPS_STORE_STL_SHFT_XMEM_H
#define TI_C54X_ISA_OPS_STORE_STL_SHFT_XMEM_H

/* Module contract
 * Owns: pure Xmem WORD16 STL-by-SHFT memory-write effects over one
 * left-shifted accumulator low word (bits 15-0 of the accumulator
 * shifted logically left by SHFT), one Xmod operand, and one Xar
 * operand. SXM rides hostile and unconsumed.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_stl_shft_xmem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_stl_shft_xmem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_STORE_STL_SHFT_XMEM_H */
