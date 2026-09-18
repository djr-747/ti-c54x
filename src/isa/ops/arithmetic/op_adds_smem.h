#ifndef TI_C54X_ISA_OPS_ARITHMETIC_ADDS_SMEM_H
#define TI_C54X_ISA_OPS_ARITHMETIC_ADDS_SMEM_H

/* Module contract
 * Owns: pure checked/trusted ADDS Smem semantics (dst(src) = src40 plus
 * zero-extended smem16) over direct page-0 and standard-indirect
 * MOD0--MOD3 reads through the memory effect, with OVM saturation plus
 * sticky OVdst and the sibling ADD C rule (C written from the bit-32
 * carry, TC preserved). Smem is always zero-extended regardless of SXM;
 * SXM is not consumed and never gates the op. Exotic indirect modes
 * (mode bits above MOD3) defer as INVALID_OPERAND. No catalog rows
 * exist for this form yet, so entry points key on UNKNOWN id with the
 * shared SELECT_INDIRECT_ADDR7 operand layout instead of opcode
 * metadata; dispatch binding follows when the rows land.
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

ti_c54x_op_status ti_c54x_op_adds_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_adds_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_ADDS_SMEM_H */
