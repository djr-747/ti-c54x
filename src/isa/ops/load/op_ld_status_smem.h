#ifndef TI_C54X_ISA_OPS_LOAD_OP_LD_STATUS_SMEM_H
#define TI_C54X_ISA_OPS_LOAD_OP_LD_STATUS_SMEM_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for fixed-MOD2 WORD16
 * indirect loads into the DP and ASM status fields. The checked leaf
 * validates the per-ID catalog/decode contract and stages a generic memory
 * read; its static completion masks the copied WORD16 and stages the status
 * field write. The trusted leaf consumes constructor-validated operands.
 * Neither path interprets input values or knownness.
 * Does not own: opcode inventory, generic decode, AR resolution or mutation,
 * buses, CPU/state mutation, sequencing, timing, observers, allocation,
 * logging, environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained; failed
 * calls leave the caller-owned effect unchanged.
 */

#include "isa/decode/opcode_decode.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_ld_dp_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_ld_asm_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOAD_OP_LD_STATUS_SMEM_H */
