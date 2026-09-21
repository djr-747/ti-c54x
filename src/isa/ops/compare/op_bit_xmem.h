#ifndef TI_C54X_ISA_OPS_COMPARE_BIT_XMEM_H
#define TI_C54X_ISA_OPS_COMPARE_BIT_XMEM_H

/* Module contract
 * Owns: checked and trusted pure BIT Xmem semantic declarations
 * (TI SPRU172C Sec4 BIT, p. 4-21: TC = Xmem(15 - BITC)). The leaf
 * stages one X-only dual-indirect WORD16 DATA READ; the read-value
 * completion stages one TC STATUS_BIT_WRITE and no other status.
 * The values entry takes the mapped AR selector (AR2..AR5), matching
 * the syntax-5/syntax-6 dual family convention.
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

ti_c54x_op_status ti_c54x_op_bit_xmem_values(
    uint8_t x_mode,
    uint8_t x_ar,
    uint8_t bitc,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_bit_xmem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_bit_xmem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_COMPARE_BIT_XMEM_H */
