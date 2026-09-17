#ifndef TI_C54X_ISA_OPS_ARITHMETIC_OP_ADD_XMEM_H
#define TI_C54X_ISA_OPS_ARITHMETIC_OP_ADD_XMEM_H

/* Module contract
 * Owns: pure checked/trusted ADD Xmem syntax-5 semantics (src +=
 * SXM-widened Xmem << SHFT, dst is always src) over the shared
 * one-word accumulator/dual-Xmod/Xar/shift4 layout with the
 * 0x9000 fixed prefix, staging an X-only dual-indirect WORD16
 * read through the memory effect, with OVM saturation plus sticky
 * OVsrc. Xmem is sign-extended iff SXM=1 else zero-extended,
 * then shifted left by the unsigned 4-bit SHFT. C is written
 * from the low-32 carry (C=1 iff carry out of bit 31, else C=0);
 * TC is preserved. No catalog rows exist for this form yet, so
 * entry points key on the shared layout plus the fixed prefix
 * instead of opcode metadata; dispatch binding follows when the
 * rows land.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../dispatch/trusted_handler.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_add_xmem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_add_xmem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_OP_ADD_XMEM_H */
