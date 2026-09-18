#ifndef TI_C54X_ISA_OPS_ARITHMETIC_SAT_ACC_H
#define TI_C54X_ISA_OPS_ARITHMETIC_SAT_ACC_H

/* Module contract
 * Owns: pure checked/trusted SAT src semantics (src40 clamped
 * to a signed 32-bit value written back to src) over the shared
 * ACCUMULATOR_SOURCE layout, regardless of OVM, with OVsrc
 * recomputed (set on clamp, cleared otherwise) and C/TC
 * preserved. The single-word opcode (0xF483 | S<<8, low byte
 * fixed 0x83) is validated directly against local constants
 * with the SAT_ACC catalog identity.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, accumulator/status mutation, buses, CPU/state
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

ti_c54x_op_status ti_c54x_op_sat_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sat_acc(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_SAT_ACC_H */
