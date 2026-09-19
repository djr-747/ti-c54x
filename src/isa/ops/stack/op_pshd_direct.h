#ifndef TI_C54X_ISA_OPS_STACK_OP_PSHD_DIRECT_H
#define TI_C54X_ISA_OPS_STACK_OP_PSHD_DIRECT_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for direct-only
 * `PSHD Smem` (page-0 dma). Both leaves validate the copied
 * bundle against the shared indirect-addr7 layout with the
 * indirect flag pinned to zero, require the copied
 * stack-pointer input, and stage one MEMORY effect with the
 * STACK_PUSH_PAIR arm (Smem DATA leg read first, TOS write
 * second). Neither path inspects live state.
 * Borrowed instruction, operand, input, and effect objects are
 * used for one call only; no pointer is retained, and failed
 * calls leave the caller effect unchanged.
 * Does not own: opcode inventory, operand-layout definitions,
 * SP state, buses, CPU or architectural state, sequencing,
 * timing, observers, diagnostics, allocation, logging,
 * environment, or callbacks.
 */

#include "../../decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_pshd_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_pshd_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STACK_OP_PSHD_DIRECT_H */
