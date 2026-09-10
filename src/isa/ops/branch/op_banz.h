#ifndef TI_C54X_ISA_OPS_BRANCH_OP_BANZ_H
#define TI_C54X_ISA_OPS_BRANCH_OP_BANZ_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for two-word BANZ
 * pmad,Sind with always-applied Sind side effects and taken
 * BRANCH_AUXILIARY versus not-taken BRANCH_AUXILIARY emission
 * carrying the AR index, value-after, update flag, taken flag, a
 * clear delayed flag, and the copied pmad. Only the
 * image-evidenced I=1/MOD0 (*ARx) and I=1/MOD1 (*ARx-)
 * combinations evaluate; every other Sind rejects as deferred.
 * Each leaf validates identity and operands and stages no status,
 * accumulator, T-register, or MEMORY effect.
 * The run loop gates CMPT=0, validates the absolute target,
 * commits the AR write when flagged, and retires the PC per the
 * taken/delay flags.
 * Does not own: catalog lookup generation, program-counter commit,
 * AR writeback, countdown arming, sequencing, timing, buses,
 * memory, diagnostics, or public ABI declarations. All inputs are
 * borrowed read-only; outputs are written only through the effect
 * out-parameter on success.
 */

#include <stdint.h>

#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_banz(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_banz(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_BANZ_H */
