#ifndef TI_C54X_ISA_OPS_ARITHMETIC_ST_PARALLEL_H
#define TI_C54X_ISA_OPS_ARITHMETIC_ST_PARALLEL_H

/*
 * Module contract
 *
 * Owns: checked and trusted pure semantic leaves for the parallel
 * ST src,Ymem || ADD/SUB Xmem,dst dual rows (TI SPRU172C
 * §ST||ADD p. 4-177, opcode 1100_00SD_XXXX_YYYY; §ST||SUB
 * p. 4-185, opcode 1100_01SD_XXXX_YYYY). See the source file
 * for the staged-effect contract.
 * Does not own: opcode inventory, operand-layout definitions,
 * address resolution or mutation, buses, CPU/state mutation,
 * sequencing, timing, observers, allocation, logging,
 * environment, or diagnostics. Borrowed instruction/input/effect
 * objects are used for one call only and no pointer is retained;
 * failed calls leave the caller-owned effect unchanged.
 */

#include <stdint.h>

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../dispatch/trusted_handler.h"
#include "../../decode/opcode_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_st_parallel_add(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_st_parallel_add(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_st_parallel_sub(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);
ti_c54x_op_status ti_c54x_trusted_op_st_parallel_sub(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_trusted_operand_values *operands,
    const ti_c54x_op_input_snapshot *input, ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_ARITHMETIC_ST_PARALLEL_H */
