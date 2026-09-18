#ifndef TI_C54X_ISA_OPS_STORE_SRCCD_XMEM_COND_H
#define TI_C54X_ISA_OPS_STORE_SRCCD_XMEM_COND_H

/* Module contract
 * Owns: pure Xmem WORD16 SRCCD conditional-store memory-RMW effects over
 * one BRC source value, one Xmod operand, one Xar operand, and one 4-bit
 * accumulator condition. The evaluated condition selects OVERWRITE
 * (cond-true stages BRC) or WRITEBACK (cond-false rewrites the read
 * word); the Xmem word is always read and updated. SXM/ASM ride hostile
 * and unconsumed.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_srccd_xmem_cond(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_srccd_xmem_cond(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_STORE_SRCCD_XMEM_COND_H */
