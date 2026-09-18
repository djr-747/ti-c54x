#ifndef TI_C54X_ISA_OPS_STORE_OP_CMPS_SMEM_DIRECT_H
#define TI_C54X_ISA_OPS_STORE_OP_CMPS_SMEM_DIRECT_H

/*
 * Module contract
 *
 * Owns: pure direct-Smem CMPS conditional max-select-store memory
 * effects over one S-selected accumulator: max(src(31:16),
 * src(15:0)) as signed 16-bit compares stages a direct Smem
 * WRITE, and the completion stages the TRN_SHIFT_WRITE
 * follow-on ((trn << 1) | taken with TC = taken). The
 * deferred indirect shape rejects transactionally.
 * Does not own: catalog rows, operand extraction, effect
 * staging, the sequencer commits that consume the effects,
 * CPU/state mutation, sequencing, timing, buses, memory,
 * diagnostics, or public ABI declarations.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_cmps_smem_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_cmps_smem_direct(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STORE_OP_CMPS_SMEM_DIRECT_H */
