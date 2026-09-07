#ifndef TI_C54X_ISA_OPS_STORE_OP_ST_T_SMEM_H
#define TI_C54X_ISA_OPS_STORE_OP_ST_T_SMEM_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the one-word fixed-MOD2
 * WORD16 store of the copied T register. The checked leaf validates the
 * catalog/decode contract and extracts ARF; both paths consume only copied T
 * knownness/value and stage one generic memory-write effect. Borrowed
 * instruction, operand, input, and effect objects are used for one call only;
 * no pointer is retained and failed calls leave the caller effect unchanged.
 * Does not own: opcode inventory, operand layouts, T state or collection,
 * address resolution, buses, CPU/state mutation, sequencing, timing,
 * observers, diagnostics, allocation, logging, or environment.
 */

#include "../../decode/opcode_decode.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_st_t_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STORE_OP_ST_T_SMEM_H */
