#ifndef TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_H
#define TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for fixed-MOD2 WORD16
 * indirect loads into accumulator A or B, including the unshifted and
 * SXM-dependent logical-left-shift-16 forms. Each checked leaf validates its
 * catalog/decode contract and stages a generic memory-read effect; its static
 * completion validates the copied instruction and input before staging the
 * accumulator write. The trusted leaves consume constructor-validated
 * operands while retaining SXM input validation.
 * Does not own: opcode inventory, generic decode, AR resolution or mutation,
 * buses, CPU/state mutation, sequencing, timing, observers, allocation,
 * logging, environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained; failed calls
 * leave the caller-owned effect unchanged.
 */

#include "isa/decode/opcode_decode.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_ld_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_ld_smem_shift16(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOAD_OP_LD_SMEM_H */
