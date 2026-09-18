#ifndef TI_C54X_ISA_OPS_MULTIPLY_MAC_LK_H
#define TI_C54X_ISA_OPS_MULTIPLY_MAC_LK_H

/* Module contract
 *
 * Owns: checked and trusted pure semantic leaves for MAC #lk,src[,dst]
 * syntax 3 (SPRU172C MAC[R] pp. 4-81-4-82): the UNROUNDED form only,
 * computing (T) times signed lk plus the S-selected source
 * accumulator into the D-selected destination accumulator through
 * the shared multiply core (FRCT shift, OVM saturation). The R
 * (rounded) suffix defers. T is READ, never written; C and TC are
 * preserved; OVdst goes sticky on 32-bit overflow (OVsrc when
 * dst==src). No catalog rows exist for this form yet, so entry
 * points key on the shared
 * ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16 layout plus
 * the 0xF067 first-word discriminator instead of opcode metadata;
 * dispatch binding follows when the rows land.
 * Does not own: catalog lookup generation, operand-layout
 * definitions, T/accumulator/status mutation, CPU sequencing,
 * timing, buses, observers, allocation, logging, environment, or
 * diagnostics. Borrowed instruction/input/effect objects are used
 * for one call only and no pointer is retained; failed calls leave
 * the caller-owned effect unchanged.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../dispatch/trusted_handler.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_mac_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_mac_lk(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_MULTIPLY_MAC_LK_H */
