#ifndef TI_C54X_ISA_OPS_LOAD_OP_LD_LK16_H
#define TI_C54X_ISA_OPS_LOAD_OP_LD_LK16_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for two-word
 * LD #lk,16,dst with the copied 16-bit extension SXM-widened and
 * shifted left 16 into the destination accumulator. Each leaf validates
 * identity and operands, requires known SXM, and stages exactly one
 * accumulator write with C/TC/overflow preserved and no OVM input,
 * following the fixed-16 shift precedent.
 * Does not own: catalog lookup generation, accumulator/status mutation,
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

ti_c54x_op_status ti_c54x_op_ld_lk16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_ld_lk16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOAD_OP_LD_LK16_H */
