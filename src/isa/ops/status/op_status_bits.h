/*
 * Module contract
 *
 * Owns: pure SSBX and RSBX status-bit effect construction from catalog-owned
 * N/SBIT operands. Effects are value-owned and committed by the status
 * sequencer; this module does not mutate status state, read inputs, perform
 * I/O, sequence instructions, or provide timing or diagnostics.
 * Does not own: catalog lookup, decode, status-register mutation, CPU
 * sequencing, timing, I/O, allocation, environment, or diagnostics.
 */

#ifndef TI_C54X_ISA_OPS_STATUS_BITS_H
#define TI_C54X_ISA_OPS_STATUS_BITS_H

#include "../op_effect.h"
#include "../op_input.h"
#include "../op_status.h"
#include "../../decode/opcode_decode.h"

ti_c54x_op_status ti_c54x_op_ssbx(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_rsbx(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

/* Catalog-token bindings used by generated checked/trusted dispatch. */
ti_c54x_op_status ti_c54x_op_ssbx_status(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_rsbx_status(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#endif /* TI_C54X_ISA_OPS_STATUS_BITS_H */
