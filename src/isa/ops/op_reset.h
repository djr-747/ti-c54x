#ifndef TI_C54X_ISA_OP_RESET_H
#define TI_C54X_ISA_OP_RESET_H

/* Module contract
 * Owns: pure RESET software-reset effects over the exact 0xF7C0
 * single word with no operands: the checked row entry with its
 * trusted twin. Both stage one RESET effect; the reset field
 * list, IFR clear, and IPTR-derived program-counter commit
 * belong to the reset applier and the CPU retirement route.
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

ti_c54x_op_status ti_c54x_op_reset(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_reset(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OP_RESET_H */
