#ifndef TI_C54X_ISA_OPS_LTD_SMEM_H
#define TI_C54X_ISA_OPS_LTD_SMEM_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the row-less LTD Smem
 * form (direct page-0 and standard-indirect MOD0--MOD3 Smem DATA reads with
 * the Smem delay write-back of the Smem word to DATA Smem+1 and a
 * T-register-write completion carrying the read word). Neither path reads
 * accumulator or status inputs and neither affects status bits.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution or mutation, buses, CPU/state mutation, sequencing, timing,
 * observers, allocation, logging, environment, or diagnostics. Borrowed
 * instruction/input/effect objects are used for one call only and no pointer
 * is retained; failed calls leave the caller-owned effect unchanged.
 */

#include "isa/decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_ltd_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_ltd_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LTD_SMEM_H */
