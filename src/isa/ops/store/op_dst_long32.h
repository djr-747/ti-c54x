#ifndef TI_C54X_ISA_OPS_STORE_OP_DST_LONG32_H
#define TI_C54X_ISA_OPS_STORE_OP_DST_LONG32_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for DST LONG32 MOD13,
 * including copied raw-40 accumulator selection, SXM/SST saturation, and a
 * two-word MSW-then-LSW memory write effect.
 * Does not own: catalog/decode tables, address resolution or mutation, bus
 * execution, CPU state, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed inputs are used for one call and
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

ti_c54x_op_status ti_c54x_op_dst_long32_mod13(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_dst_long32_mod13(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STORE_OP_DST_LONG32_H */
