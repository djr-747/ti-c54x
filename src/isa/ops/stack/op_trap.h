#ifndef TI_C54X_ISA_OPS_STACK_OP_TRAP_H
#define TI_C54X_ISA_OPS_STACK_OP_TRAP_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the one-word
 * `TRAP K` software interrupt. The checked leaf validates the copied
 * instruction and extracts its 5-bit K field; both leaves require
 * the copied stack-pointer input and stage one STACK_PUSH_TRAP
 * MEMORY effect with a zero staged value and null completion
 * (the root freezes PC+1 and the K vector). Neither path inspects
 * live state. Borrowed instruction, operand, input, and effect
 * objects are used for one call only; no pointer is retained, and
 * failed calls leave the caller effect unchanged.
 * Does not own: opcode inventory, operand-layout definitions,
 * SP/IPTR state, buses, CPU or architectural state, sequencing,
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

ti_c54x_op_status ti_c54x_op_trap(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_trap(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STACK_OP_TRAP_H */
