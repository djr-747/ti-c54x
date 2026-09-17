#ifndef TI_C54X_ISA_OPS_MULTIPLY_MASA_SMEM_H
#define TI_C54X_ISA_OPS_MULTIPLY_MASA_SMEM_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for MASA Smem
 * (direct page-0 and standard-indirect MOD0--MOD3 reads through
 * the memory effect with read-phase T load and read-value SUBTRACT
 * completion of B minus Smem times A-high into fixed B through
 * the multiply core). A-high is the multiplicand, so input A
 * must be known, and B must be known for the subtract; T arrives
 * via the read-phase load and needs no prior knownness. Syntax 1
 * carries no rounding select.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include <stdint.h>

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../dispatch/trusted_handler.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_masa_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_masa_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_MULTIPLY_MASA_SMEM_H */
