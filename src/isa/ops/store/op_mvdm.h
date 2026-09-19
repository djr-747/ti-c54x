#ifndef TI_C54X_ISA_OPS_STORE_OP_MVDM_H
#define TI_C54X_ISA_OPS_STORE_OP_MVDM_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the two-word
 * absolute Smem-to-MMR move. The checked leaf validates the
 * copied instruction and extracts its 5-bit MMR destination and
 * 16-bit absolute Smem address; the move stages one absolute
 * DATA READ effect with the read-phase MMR store sidecar. Neither
 * path inspects input values or knownness. Borrowed instruction,
 * operand, input, and effect objects are used for one call only;
 * no pointer is retained, and failed calls leave the caller
 * effect unchanged.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution, buses, CPU or architectural state,
 * sequencing, timing, observers, diagnostics, allocation,
 * logging, environment, or callbacks.
 */

#include "../../decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_mvdm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_mvdm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STORE_OP_MVDM_H */
