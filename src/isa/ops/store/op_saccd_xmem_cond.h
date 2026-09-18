#ifndef TI_C54X_ISA_OPS_STORE_SACCD_XMEM_COND_H
#define TI_C54X_ISA_OPS_STORE_SACCD_XMEM_COND_H

/* Module contract
 * Owns: pure dual-X Xmem WORD16 SACCD conditional-store memory effects
 * over one S-selected accumulator shifted by (ASM-16) with SXM-gated
 * right-shift fill, one Xmod operand, one Xar operand, and one 4-bit
 * accumulator condition. Cond-true stages a READ with the OVERWRITE
 * transform carrying the low 16 of the shifted source (TI SPRU172C
 * p. 4-153: A FF_FE00_4321 with ASM=01 stages 0xFC00); cond-false
 * stages the same READ with the WRITEBACK transform that rewrites the
 * read word and ignores the staged value. Condition evaluation is the
 * shared store-owned S48 evaluator over borrowed inputs.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_saccd_xmem_cond(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_saccd_xmem_cond(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_STORE_SACCD_XMEM_COND_H */
