#ifndef TI_C54X_ISA_OP_INPUT_H
#define TI_C54X_ISA_OP_INPUT_H

/*
 * Module contract
 *
 * Owns: the private, copied read-only input carrier supplied to every opcode
 * handler. The snapshot is borrowed for one call and no handler may retain
 * its address; raw40 values, auxiliary-register raw16 values, T raw16,
 * stack-pointer raw16, return-register raw16, and status C/TC/SXM/SST/OVM/
 * ASM/FRCT values are canonical and knownness is explicit.
 * Does not own: architectural storage, decode, effects, sequencing, CPU
 * lifecycle, buses, timing, diagnostics, or public persistence formats.
 */

#include <stdbool.h>
#include <stdint.h>

#include "foundation/arithmetic40.h"

typedef struct ti_c54x_op_input_snapshot {
    ti_c54x_u40_t accumulator_a_raw40;
    ti_c54x_u40_t accumulator_b_raw40;
    uint16_t auxiliary_registers_raw16[8];
    uint16_t t_register_raw16;
    uint16_t stack_pointer_raw16;
    bool stack_pointer_known;
    uint16_t return_register_raw16;
    bool return_register_known;
    uint8_t auxiliary_registers_known_mask;
    bool accumulator_a_known;
    bool accumulator_b_known;
    uint8_t status_c;
    bool status_c_known;
    uint8_t status_tc;
    bool status_tc_known;
    uint8_t status_sxm;
    bool status_sxm_known;
    bool t_register_known;
    uint8_t status_sst;
    bool status_sst_known;
    uint8_t status_ovm;
    bool status_ovm_known;
    uint8_t status_asm;
    bool status_asm_known;
    uint8_t status_frct;
    bool status_frct_known;
} ti_c54x_op_input_snapshot;

#endif /* TI_C54X_ISA_OP_INPUT_H */
