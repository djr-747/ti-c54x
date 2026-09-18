#ifndef TI_C54X_ISA_OPS_ARITHMETIC_SUBC_SMEM_H
#define TI_C54X_ISA_OPS_ARITHMETIC_SUBC_SMEM_H

/* Module contract
 * Owns: pure checked/trusted SUBC Smem semantics (conditional-subtract
 * divide step) over direct page-0 and standard-indirect MOD0--MOD3
 * reads through the memory effect. SXM gates the Smem sign handling
 * exactly like SUBB Smem, except SXM==1 with the Smem MSB set rejects
 * as INVALID_OPERAND; C is written from the t > 0 test, the stored
 * value is never saturated (SUBC ignores OVM), the written
 * accumulator's OV bit is sticky-set iff the stored value misfits
 * signed-32, and TC is preserved. Exotic indirect modes (mode bits
 * above MOD3) defer as INVALID_OPERAND. No catalog row exists for
 * this form yet, so the checked entry keys on UNKNOWN id, the shared
 * SELECT_INDIRECT_ADDR7 operand layout, a single program word, and
 * the (first_word & 0xFE00) == 0x1E00 pattern instead of opcode
 * metadata; dispatch binding follows when the row lands.
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

ti_c54x_op_status ti_c54x_op_subc_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_subc_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_SUBC_SMEM_H */
