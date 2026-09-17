#ifndef TI_C54X_ISA_OPS_LOGICAL_XOR_SMEM_H
#define TI_C54X_ISA_OPS_LOGICAL_XOR_SMEM_H

/* Module contract
 * Owns: checked and trusted pure XOR Smem semantics for the single
 * FORM-T2-09-01 row (TI SPRU172C Sec4 XOR, p. 4-201: dst ^= Smem)
 * over direct page-0 and standard-indirect MOD0--MOD3 reads through
 * the memory effect. The 16-bit Smem word is zero-extended unshifted
 * and XORed full-width with the destination accumulator; C and TC
 * are preserved and no SXM/OVM input is consumed. Exotic indirect
 * modes (mode bits above MOD3) defer as INVALID_OPERAND.
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

ti_c54x_op_status ti_c54x_op_xor_smem_values(
    uint8_t destination,
    uint8_t indirect,
    uint8_t addr7,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_xor_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOGICAL_XOR_SMEM_H */
