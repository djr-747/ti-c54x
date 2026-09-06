#ifndef TI_C54X_INDIRECT_CIRCULAR_H
#define TI_C54X_INDIRECT_CIRCULAR_H

/*
 * Module contract
 *
 * Owns: checked CMPT=0 fixed-step circular WORD16 effective-address
 * planning for MOD8 (post-access decrement) and MOD10 (post-access
 * increment), including the value-owned AR/BK snapshots and intrinsic effect
 * validation.
 * Does not own: CPU/register storage, ARP/CMPT/BK dependency validation,
 * memory or bus transfers, opcode decoding, instruction sequencing, timing,
 * diagnostics, or any other circular mode. Inputs are copied scalar values;
 * results own no borrowed storage and every non-OK result has an all-zero
 * effect. A zero BK value is a successful architectural no-op update while
 * still retaining the circular source snapshots.
 */

#include <stdbool.h>
#include <stdint.h>

#include "address_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_indirect_circular_mode {
    TI_C54X_INDIRECT_CIRCULAR_MOD8 = 8,
    TI_C54X_INDIRECT_CIRCULAR_MOD10 = 10
} ti_c54x_indirect_circular_mode;

/*
 * Plan one CMPT=0 single-indirect WORD16 operand using fixed-step circular
 * addressing. The operand is the old ARx value. MOD8 decrements the selected
 * ARx after access; MOD10 increments it after access. BK is copied into the
 * source snapshot and is never read from architectural state by this module.
 * For positive BK, an initial AR outside the documented circular buffer is
 * returned as TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY. BK=0 returns the
 * old-AR operand with a canonical NONE update.
 * Invalid selector, mode, or access discriminants return INVALID_ARGUMENT
 * before any positive-BK buffer-domain decision. Every non-OK result carries
 * the all-zero effect promised by this module contract.
 * The caller/sequencer must validate CMPT/ARP/BK dependencies before applying
 * the returned effect.
 */
ti_c54x_address_result ti_c54x_indirect_circular_word16(
    uint8_t ar_index,
    uint16_t ar_value,
    uint16_t bk_value,
    ti_c54x_indirect_circular_mode mode,
    ti_c54x_address_access access);

/* Validate only the intrinsic, value-owned shape of a circular WORD16 effect.
 * The effect is borrowed read-only for this call; no pointer is retained. */
bool ti_c54x_indirect_circular_effect_is_valid(
    const ti_c54x_address_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_INDIRECT_CIRCULAR_H */
