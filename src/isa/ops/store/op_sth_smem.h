#ifndef TI_C54X_ISA_OPS_STORE_STH_SMEM_H
#define TI_C54X_ISA_OPS_STORE_STH_SMEM_H

/* Module contract
 * Owns: pure standard-indirect WORD16 STH memory-write effects over one
 * selected canonical accumulator high word (bits 31-16), one ARF operand, and one MOD
 * operand.
 * Does not own: opcode inventory, operand-layout definitions, address
 * resolution, buses, CPU or architectural state, sequencing, timing,
 * observers, diagnostics, allocation, or callbacks.
 */

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_sth_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_sth_smem_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_sth_smem_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_sth_smem_mod8(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_sth_smem_mod10(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_sth_smem_mod4(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_sth_smem_mod7(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sth_smem_mod5(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sth_smem_mod6(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sth_smem_mod8(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_sth_smem_mod10(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_STORE_STH_SMEM_H */
