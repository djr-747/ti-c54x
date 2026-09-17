#ifndef TI_C54X_ISA_OPS_MULTIPLY_MACA_T_H
#define TI_C54X_ISA_OPS_MULTIPLY_MACA_T_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for MACA T,src
 * (register-only ADD of the S-selected source accumulator plus T
 * times A-high into the D-selected accumulator through the
 * multiply core with R rounding). T, A, and src must be known;
 * T and the unselected accumulator are preserved and C/TC ride
 * through unchanged.
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

ti_c54x_op_status ti_c54x_op_maca_t(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_maca_t(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_MULTIPLY_MACA_T_H */
