#ifndef TI_C54X_ISA_OP_IDLE_H
#define TI_C54X_ISA_OP_IDLE_H

/* Module contract
 * Owns: pure IDLE power-down effects over the 2-bit K selector
 * (K = NN+1, power-down modes 1-3): the checked row entry with
 * its trusted twin. The reserved NN=3 selector rejects.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution, buses, CPU or architectural state,
 * sequencing, timing, observers, diagnostics, allocation,
 * or callbacks.
 */

#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"

#include "op_effect.h"
#include "op_input.h"
#include "op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_idle(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_idle(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OP_IDLE_H */
