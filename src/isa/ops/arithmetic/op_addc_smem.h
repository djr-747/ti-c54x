#ifndef TI_C54X_ISA_OPS_ARITHMETIC_ADDC_SMEM_H
#define TI_C54X_ISA_OPS_ARITHMETIC_ADDC_SMEM_H

/* Module contract
 * Owns: pure checked/trusted ADDC Smem semantics
 * (dst(src) = src + zero-extended Smem + C) over direct page-0 and
 * standard-indirect MOD0--MOD3 reads through the memory effect,
 * with SXM-suppressed Smem widening, C written from the bit-32
 * carry, and OVM saturation plus sticky OVdst. Exotic indirect
 * modes (mode bits above MOD3) defer as INVALID_OPERAND. No
 * catalog row exists for this form yet, so the checked entry keys
 * on the UNKNOWN opcode id plus the 0x0600/0xFE00 first-word gate
 * through the shared SELECT_INDIRECT_ADDR7 operand layout;
 * dispatch binding follows when the row lands.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, address resolution or mutation, buses, CPU/state
 * mutation, sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../dispatch/trusted_handler.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_addc_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_addc_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_ADDC_SMEM_H */
