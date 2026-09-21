#ifndef TI_C54X_ISA_OPS_STORE_OP_MVDD_XMEM_YMEM_H
#define TI_C54X_ISA_OPS_STORE_OP_MVDD_XMEM_YMEM_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the
 * single-word dual-indirect DATA move (TI SPRU172C section
 * MVDD, p. 4-107). The checked leaf validates the copied
 * 0xE500 instruction and extracts the XMOD/XAR/YMOD/YAR
 * fields; both entries stage one DUAL_INDIRECT_MOVE_PAIR
 * effect (X DATA leg read first, Y DATA leg WRITE second
 * with the observed X word, zero staged value, no
 * completion) over MOD0--MOD3 modes with AR2..AR5
 * selectors. Borrowed instruction, operand, input, and
 * effect objects are used for one call only; no pointer is
 * retained, and failed calls leave the caller effect
 * unchanged.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution, buses, CPU or architectural state,
 * sequencing, timing, observers, diagnostics, allocation,
 * logging, environment, or callbacks.
 */

#include "../../decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_mvdd_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_mvdd_xmem_ymem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_STORE_OP_MVDD_XMEM_YMEM_H */
