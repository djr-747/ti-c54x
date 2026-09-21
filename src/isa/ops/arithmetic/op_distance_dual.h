#ifndef TI_C54X_ISA_OPS_ARITHMETIC_OP_DISTANCE_DUAL_H
#define TI_C54X_ISA_OPS_ARITHMETIC_OP_DISTANCE_DUAL_H

/* Module contract
 * Owns: checked and trusted dual-memory SQDST/ABDST semantic leaves
 * over the one-word 0xE200/0xE300 dual-XMOD/XAR/YMOD/YAR forms. The
 * leaves stage one dual WORD16 read effect carrying both sides'
 * modes and, on pair completion, one accumulator-write pair
 * (A stages (X-Y)<<16; B accumulates the distance term).
 * Does not own: catalog generation, operand layout definitions,
 * address resolution, memory/bus execution, architectural mutation,
 * sequencing, timing, observers, diagnostics, or allocation.
 * Borrowed instruction, input, and effect objects are used for one
 * call only.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"
#include "../dispatch/trusted_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_sqdst_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_abdst_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sqdst_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_abdst_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_OP_DISTANCE_DUAL_H */
