#ifndef TI_C54X_ISA_OPS_ARITHMETIC_OP_LMS_DUAL_H
#define TI_C54X_ISA_OPS_ARITHMETIC_OP_LMS_DUAL_H

/* Module contract
 * Owns: checked and trusted dual-memory LMS semantic leaves
 * over the one-word 0xC100 dual-XMOD/XAR/YMOD/YAR form. The
 * leaves stage one dual WORD16 read effect carrying both sides'
 * modes and, on pair completion, one accumulator-write pair
 * (A accumulates (X<<16)+2^15 as one 40-bit add; B accumulates
 * the signed X*Y product, FRCT-doubled).
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

ti_c54x_op_status ti_c54x_op_lms_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_lms_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_OP_LMS_DUAL_H */
