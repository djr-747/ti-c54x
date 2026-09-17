#ifndef TI_C54X_ISA_OPS_MULTIPLY_SQUR_ADST_H
#define TI_C54X_ISA_OPS_MULTIPLY_SQUR_ADST_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for SQUR A,dst
 * syntax 2 (register-only A-high squared into the D-selected
 * accumulator through the multiply core with STORE select and no
 * rounding). A, FRCT, and OVM gate staging; T is never read or
 * written by the leaf.
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

ti_c54x_op_status ti_c54x_op_squr_adst(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_squr_adst(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_MULTIPLY_SQUR_ADST_H */
