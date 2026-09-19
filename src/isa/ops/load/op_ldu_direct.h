#ifndef TI_C54X_ISA_OPS_LOAD_OP_LDU_DIRECT_H
#define TI_C54X_ISA_OPS_LOAD_OP_LDU_DIRECT_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for DP-relative
 * direct WORD16 unsigned loads into accumulator A or B. The checked
 * leaf validates the catalog/decode contract, consumes the D
 * destination bit plus the 7-bit direct cookie, gates DP
 * knownness, and stages a generic memory-read effect; its static
 * completion zero-extends the copied WORD16 into a canonical
 * 40-bit accumulator write. The trusted leaf consumes
 * constructor-validated operands. Neither path observes or
 * interprets SXM. Does not own: opcode inventory, generic decode,
 * DP resolution, buses, CPU/state mutation, sequencing, timing,
 * observers, allocation, logging, environment, or diagnostics.
 * Borrowed instruction/input/effect objects are used for one call
 * only and no pointer is retained; failed calls leave the
 * caller-owned effect unchanged.
 */

#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_ldu_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_ldu_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOAD_OP_LDU_DIRECT_H */
