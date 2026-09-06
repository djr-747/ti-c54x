#ifndef TI_C54X_INDIRECT_REVERSE_CARRY_H
#define TI_C54X_INDIRECT_REVERSE_CARRY_H

/*
 * Module contract
 *
 * Owns: checked CMPT=0 WORD16 reverse-carry MOD4/MOD7 effective-address
 * planning, including copied selected-AR/AR0 sources and one POST_ACCESS
 * selected-AR update.
 * Does not own: CPU/register storage, ARP/CMPT validation, register mutation,
 * memory or bus transfers, opcode decoding, LONG32/circular addressing,
 * timing, or diagnostics. Inputs are copied scalar values and results retain
 * no borrowed storage. Every rejected planner result has an all-zero effect.
 */

#include <stdbool.h>
#include <stdint.h>

#include "address_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_indirect_reverse_carry_mode {
    TI_C54X_INDIRECT_REVERSE_CARRY_MOD4 = 4,
    TI_C54X_INDIRECT_REVERSE_CARRY_MOD7 = 7
} ti_c54x_indirect_reverse_carry_mode;

/* Plan one CMPT=0 ARx reverse-carry WORD16 operand. MOD4 subtracts the
 * one-hot AR0 value from ARx; MOD7 adds it. The old ARx addresses the operand
 * and the selected AR update is committed after access by the sequencer. */
ti_c54x_address_result ti_c54x_indirect_reverse_carry_word16(
    uint8_t ar_index,
    uint16_t ar_value,
    uint16_t ar0_value,
    ti_c54x_indirect_reverse_carry_mode mode,
    ti_c54x_address_access access);

/* Validate the intrinsic reverse-carry effect shape without reading CPU state.
 * The effect is borrowed read-only until return and no pointer is retained. */
bool ti_c54x_indirect_reverse_carry_effect_is_valid(
    const ti_c54x_address_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_INDIRECT_REVERSE_CARRY_H */
