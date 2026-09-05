#ifndef TI_C54X_INDIRECT_SINGLE_H
#define TI_C54X_INDIRECT_SINGLE_H

/*
 * Module contract
 *
 * Owns: checked CMPT=0 single-indirect word and long32 effective-address
 * planning for MOD0 through MOD3.
 * Does not own: CPU state or ARP/CMPT state validation, bus transfers, opcode
 * decoding, circular or bit-reversed addressing, dual-indirect forms,
 * instruction scheduling, or diagnostics.
 * Inputs are copied scalar values. The returned status/effect owns no borrowed
 * storage and remains valid after the call returns. Every successful result
 * carries `TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0`; the planner does not
 * inspect or validate CPU state. Every non-OK result has an all-zero effect.
 */

#include <stdint.h>

#include "address_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_indirect_single_mode {
    TI_C54X_INDIRECT_SINGLE_MOD0 = 0,
    TI_C54X_INDIRECT_SINGLE_MOD1 = 1,
    TI_C54X_INDIRECT_SINGLE_MOD2 = 2,
    TI_C54X_INDIRECT_SINGLE_MOD3 = 3
} ti_c54x_indirect_single_mode;

/*
 * Plan one CMPT=0 single-indirect word operand. The caller supplies the ARF
 * selected auxiliary-register index and its current 16-bit word address. The
 * planner does not mutate caller state or consult ARP/CMPT; a sequencer must
 * validate the carried CMPT0/ARP0 policy before applying `effect` in the phase
 * recorded by its update member.
 */
ti_c54x_address_result ti_c54x_indirect_single_word(
    uint8_t ar_index,
    uint16_t ar_value,
    ti_c54x_indirect_single_mode mode,
    ti_c54x_address_access access);

/*
 * Plan one CMPT=0 single-indirect 32-bit data operand. The first address is
 * the accessed MSW address; the descriptor supplies the paired LSW address.
 * MOD1/MOD2/MOD3 apply a two-word AR delta, while MOD3 remains write-only.
 */
ti_c54x_address_result ti_c54x_indirect_single_long32(
    uint8_t ar_index,
    uint16_t ar_value,
    ti_c54x_indirect_single_mode mode,
    ti_c54x_address_access access);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_INDIRECT_SINGLE_H */
