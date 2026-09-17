#ifndef TI_C54X_ISA_OPS_ARITHMETIC_ADD_SRC_ASM_H
#define TI_C54X_ISA_OPS_ARITHMETIC_ADD_SRC_ASM_H

/* Module contract
 * Owns: pure checked/trusted ADD src,ASM semantics for syntax 10
 * (dst40 = dst40 plus the accumulator source shifted by the signed
 * 5-bit ASM count) over the shared
 * ACCUMULATOR_SOURCE_DESTINATION layout, with SXM-dependent shifts,
 * OVM saturation plus sticky OVdst, C written from the bit-32 carry,
 * and TC preserved. The single-word opcode (0xF480 | S<<9 | D<<8,
 * low byte fixed 0x80) is validated directly because no catalog row
 * exists for this form yet; dispatch binding follows when the row
 * lands. An explicit dst selects the addend base; an omitted dst is
 * assembled with D == S so the base is the source itself.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, accumulator/status mutation, buses, CPU/state
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

ti_c54x_op_status ti_c54x_op_add_src_asm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_add_src_asm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_ADD_SRC_ASM_H */
