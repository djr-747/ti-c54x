#ifndef TI_C54X_ISA_OPS_ARITHMETIC_OP_DSUB_H
#define TI_C54X_ISA_OPS_ARITHMETIC_OP_DSUB_H

/* Module contract
 * Owns: pure double-precision DSUB Lmem effects: one kind-21
 * LONG32 READ over a page-0 direct or standard-indirect MOD0-MOD3
 * Lmem leg, plus the C16-gated completion (40-bit src minus
 * Lmem with SXM/OVM/C/OVsrc at C16=0; dual src-Lmem high and
 * low subtracts, guard sign-filled from result bit 31, C from
 * the clean low half, no saturation at C16=1).
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_dsub(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_dsub(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_OP_DSUB_H */
