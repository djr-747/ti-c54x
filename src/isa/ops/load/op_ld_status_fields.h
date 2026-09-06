#ifndef TI_C54X_ISA_OPS_LOAD_OP_LD_STATUS_FIELDS_H
#define TI_C54X_ISA_OPS_LOAD_OP_LD_STATUS_FIELDS_H

/*
 * Module contract
 *
 * Owns: pure semantic handlers for the three immediate loads that write DP,
 * ASM, or ARP status fields. Each handler validates its catalog descriptor,
 * extracts a copied raw field, and returns a value-owned status-field effect.
 * Does not own: opcode inventory, generic decode, CPU/state mutation, status
 * commit, program-counter sequencing, repeat policy, timing, memory, buses,
 * or diagnostics. Caller-owned instruction/effect storage is borrowed for one
 * call and is never retained; failed calls leave the effect unchanged.
 */

#include "isa/decode/opcode_decode.h"
#include "isa/ops/op_effect.h"
#include "isa/ops/op_input.h"
#include "isa/ops/op_status.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_op_status ti_c54x_op_ld_immediate_dp(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_ld_immediate_asm(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

ti_c54x_op_status ti_c54x_op_ld_immediate_arp(
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    ti_c54x_op_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_ISA_OPS_LOAD_OP_LD_STATUS_FIELDS_H */
