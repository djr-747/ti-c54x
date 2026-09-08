#ifndef TI_C54X_ISA_OPS_ARITHMETIC_OP_DUAL_ARITHMETIC_H
#define TI_C54X_ISA_OPS_ARITHMETIC_OP_DUAL_ARITHMETIC_H

/* Module contract
 * Owns: checked and trusted syntax-6 dual-memory ADD/SUB MOD0 semantic
 * leaves.  The leaves stage one dual WORD16 read effect and, on pair
 * completion, one accumulator write with fixed-left-16 arithmetic, OVM
 * saturation, sticky destination overflow, and fixed-shift carry policy.
 * Does not own: catalog generation, operand layout definitions, address
 * resolution, memory/bus execution, architectural mutation, sequencing,
 * timing, observers, diagnostics, or allocation.  Borrowed instruction,
 * input, and effect objects are used for one call only.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"
#include "../dispatch/trusted_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_add_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_sub_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_add_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sub_dual_mod0(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_OP_DUAL_ARITHMETIC_H */
