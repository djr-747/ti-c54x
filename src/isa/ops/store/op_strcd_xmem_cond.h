#ifndef TI_C54X_ISA_OPS_STORE_STRCD_XMEM_COND_H
#define TI_C54X_ISA_OPS_STORE_STRCD_XMEM_COND_H

/* Module contract
 * Owns: pure Xmem WORD16 STRCD conditional-store memory-read effects
 * over the T register source, one Xmod operand, one Xar operand,
 * and one 4-bit accumulator condition. Cond-true stages OVERWRITE
 * with the T word and cond-false stages WRITEBACK with an ignored
 * zero word; neither stages flags. S is fixed zero (S=1 selects
 * SRCCD) and no shift, ASM, or SXM input is consumed.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_strcd_xmem_cond(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_strcd_xmem_cond(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_STORE_STRCD_XMEM_COND_H */
