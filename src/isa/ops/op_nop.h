#ifndef TI_C54X_ISA_OP_NOP_H
#define TI_C54X_ISA_OP_NOP_H

/*
 * Module contract
 *
 * Owns: direct semantic validation and execution for the catalog NOP.
 * Does not own: catalog inventory, opcode decoding, program-counter changes,
 * CPU/register state, status state, memory, buses, timing, sequencing, or
 * observers. Inputs are borrowed for the duration of the call; the caller
 * owns the output effect and it is written only after successful validation.
 */

#include "isa/decode/opcode_decode.h"

#include "op_effect.h"
#include "op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_nop(
    const ti_c54x_decode_result *instruction,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OP_NOP_H */
