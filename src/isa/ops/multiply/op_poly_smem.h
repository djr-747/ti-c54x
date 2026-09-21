#ifndef TI_C54X_ISA_OPS_MULTIPLY_OP_POLY_SMEM_H
#define TI_C54X_ISA_OPS_MULTIPLY_OP_POLY_SMEM_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the one-word
 * `POLY Smem` polynomial step. The checked leaf validates the copied
 * instruction and extracts the indirect/addr7 fields; both leaves
 * require the copied accumulators, live T, and SXM/FRCT/OVM status
 * and stage one single-memory DATA READ with no T load. The
 * read-value completion rounds A-high times T plus B-old into A
 * (FRCT-doubled, OVM-saturated, OVA on destination overflow)
 * and loads the SXM-widened Smem word shifted left 16 into B,
 * publishing one ACCUMULATOR_WRITE_PAIR. Neither path inspects
 * live state. Borrowed instruction, operand, input, and effect
 * objects are used for one call only; no pointer is retained, and
 * failed calls leave the caller effect unchanged.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics.
 */

#include "../../decode/opcode_decode.h"
#include "isa/ops/dispatch/trusted_semantics.h"
#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_poly_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_trusted_op_poly_smem(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_MULTIPLY_OP_POLY_SMEM_H */
