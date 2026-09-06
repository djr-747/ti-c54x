#ifndef TI_C54X_SEQUENCER_ADDRESS_EFFECT_VALIDATE_H
#define TI_C54X_SEQUENCER_ADDRESS_EFFECT_VALIDATE_H

/*
 * Module contract
 *
 * Owns: the composite read-only validation entry point and the minimal commit
 * description consumed by address_effect_apply.c. Intrinsic effect checks and
 * architectural dependency reads are delegated to their private owners.
 * Does not own: intrinsic shape rules, architectural state reads or mutation,
 * planning arithmetic, opcode decode, CPU lifecycle, buses, memory, timing,
 * diagnostics, or public ABI.
 */

#include <stdint.h>

#include "address_effect_apply.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The only architectural mutation an accepted effect may request. */
typedef struct ti_c54x_sequencer_address_commit {
    ti_c54x_address_update_target target;
    uint8_t ar_index;
    uint16_t value_after;
} ti_c54x_sequencer_address_commit;

/*
 * Validate an effect without mutating state.  `state` and `result` are
 * borrowed for this call and `out_commit` is caller-owned.  A non-NULL output
 * is zeroed before validation and remains zero on every rejection.  A NONE
 * effect returns an all-zero commit on success; an AR effect returns the
 * single selected-AR write that the application layer may perform.
 */
ti_c54x_address_apply_status ti_c54x_sequencer_validate_address_effect(
    const ti_c54x_arch_state *state,
    const ti_c54x_address_result *result,
    ti_c54x_address_update_phase requested_phase,
    ti_c54x_sequencer_address_commit *out_commit);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_ADDRESS_EFFECT_VALIDATE_H */
