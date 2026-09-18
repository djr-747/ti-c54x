#ifndef TI_C54X_ISA_OPS_COMPARE_BITT_SMEM_H
#define TI_C54X_ISA_OPS_COMPARE_BITT_SMEM_H

/* Module contract
 * Owns: checked and trusted pure BITT Smem semantic declarations
 * (TI SPRU172C Sec4 BITT, p. 4-23: TC = Smem(15 - T(3-0))). The leaf
 * stages one WORD16 Smem read gated on a known T register with no T
 * load; the read-value completion stages one TC STATUS_BIT_WRITE and
 * no other status.
 * Does not own: catalog rows, operand-layout definitions, decode,
 * dispatch binding, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include <stdint.h>

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../dispatch/trusted_handler.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_bitt_smem_values(
    uint8_t indirect,
    uint8_t addr7,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_bitt_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_bitt_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_COMPARE_BITT_SMEM_H */
