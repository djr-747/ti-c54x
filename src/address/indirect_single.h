#ifndef TI_C54X_INDIRECT_SINGLE_H
#define TI_C54X_INDIRECT_SINGLE_H

/*
 * Module contract
 *
 * Owns: checked CMPT=0 single-indirect word effective-address planning for
 * MOD0 through MOD3.
 * Does not own: CPU state, ARP/CMPT state validation, bus transfers, opcode
 * decoding, long operands, circular or bit-reversed addressing, dual-indirect
 * forms, instruction scheduling, or diagnostics.
 * Inputs are copied scalar values. The returned status/effect owns no borrowed
 * storage and remains valid after the call returns. Every non-OK result has an
 * all-zero effect.
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
 * planner does not mutate caller state or consult ARP; a sequencer may later
 * apply `effect` in the phase recorded by its update member.
 */
ti_c54x_address_result ti_c54x_indirect_single_word(
    uint8_t ar_index,
    uint16_t ar_value,
    ti_c54x_indirect_single_mode mode,
    ti_c54x_address_access access);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_INDIRECT_SINGLE_H */
