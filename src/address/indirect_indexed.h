#ifndef TI_C54X_INDIRECT_INDEXED_H
#define TI_C54X_INDIRECT_INDEXED_H

/*
 * Module contract
 *
 * Owns: checked CMPT=0 single-operand WORD16 effective-address planning for
 * AR0-indexed MOD5 and MOD6 forms, including their one selected-AR update.
 * Does not own: CPU state or ARP/CMPT validation, register storage or
 * mutation, bus transfers, opcode decoding, wrapping/circular arithmetic,
 * long32 operands, instruction scheduling, or diagnostics.
 * Inputs are copied scalar values. Results are returned by value and retain no
 * borrowed storage. Every successful effect carries the CMPT0/ARP0 state
 * policy; every rejected input returns an all-zero effect.
 */

#include <stdint.h>

#include "address_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_indirect_indexed_mode {
    TI_C54X_INDIRECT_INDEXED_MOD5 = 5,
    TI_C54X_INDIRECT_INDEXED_MOD6 = 6
} ti_c54x_indirect_indexed_mode;

/*
 * Plan one CMPT=0 AR0-indexed WORD16 operand. The operand uses the old value
 * of selected ARx. MOD5 schedules ARx := ARx - AR0 and MOD6 schedules
 * ARx := ARx + AR0 after access. AR0 itself is copied into the effect but is
 * never updated. The planner does not inspect or mutate CPU state.
 */
ti_c54x_address_result ti_c54x_indirect_indexed_word(
    uint8_t ar_index,
    uint16_t ar_value,
    uint16_t ar0_value,
    ti_c54x_indirect_indexed_mode mode,
    ti_c54x_address_access access);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_INDIRECT_INDEXED_H */
