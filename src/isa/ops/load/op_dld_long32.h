#ifndef TI_C54X_ISA_OPS_LOAD_OP_DLD_LONG32_H
#define TI_C54X_ISA_OPS_LOAD_OP_DLD_LONG32_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the DLD LONG32 MOD13
 * two-word load.  The leaves stage a copied CMPT=0 MOD13 memory read and a
 * static completion that composes MSW/LSW with canonical SXM extension.
 * Does not own: catalog/decode tables, address resolution or mutation, bus
 * execution, CPU state, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.  Borrowed inputs are used for one call and
 * failed calls leave caller-owned effects unchanged.
 */

#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_dld_long32_mod13(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_dld_long32_mod13(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOAD_OP_DLD_LONG32_H */
