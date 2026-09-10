#ifndef TI_C54X_ISA_OPS_BRANCH_OP_BC_PMAD_H
#define TI_C54X_ISA_OPS_BRANCH_OP_BC_PMAD_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for two-word
 * BC pmad,cond with the sixteen image-evidenced single conditions
 * (TC/NTC/C/NC plus twelve full-40-bit signed A/B compares).
 * A taken condition stages PC_WRITE with the copied 16-bit pmad; a
 * not-taken condition stages effect NONE for the sequential bridge.
 * Each leaf validates identity and operands, demands knownness only
 * for consumed inputs, and rejects deferred condition codes
 * transactionally.
 * Does not own: catalog lookup generation, program-counter commit,
 * sequencing, timing, buses, memory, diagnostics, or public ABI
 * declarations. All inputs are borrowed read-only; outputs are written
 * only through the effect out-parameter on success.
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

ti_c54x_op_status ti_c54x_op_bc_pmad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_bc_pmad(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_BRANCH_OP_BC_PMAD_H */
