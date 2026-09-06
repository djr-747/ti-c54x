#ifndef TI_C54X_ISA_OPCODE_DISPATCH_H
#define TI_C54X_ISA_OPCODE_DISPATCH_H

/*
 * Module contract
 *
 * Owns: the private function-pointer handler type and catalog-derived
 * semantic dispatch seam.
 * Does not own: opcode inventory, decode, CPU/register state, program-counter
 * sequencing, timing, memory, buses, or diagnostics. The descriptor is
 * borrowed for the call; output is caller-owned and is committed only when a
 * selected handler succeeds.
 */

#include "isa/decode/opcode_decode.h"

#include "op_effect.h"
#include "op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ti_c54x_op_status (*ti_c54x_op_handler)(
    const ti_c54x_decode_result *instruction,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_dispatch_decoded(
    const ti_c54x_decode_result *instruction,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPCODE_DISPATCH_H */
