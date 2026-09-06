/*
 * Module contract
 *
 * Owns: the ordered validation transaction that combines state-free intrinsic
 * checks with read-only architectural dependency checks, then publishes one
 * private commit descriptor. It never mutates architectural state.
 * Does not own: validation arithmetic or shape rules (intrinsic owner),
 * architectural reads (dependency owner), address planning, opcode decoding,
 * CPU lifecycle, buses, memory, timing, diagnostics, or public ABI.
 */

#include "address_effect_validate.h"

#include "address_effect_dependencies.h"
#include "address_effect_intrinsic.h"

#include <stddef.h>
#include <string.h>

ti_c54x_address_apply_status
ti_c54x_sequencer_validate_address_effect(
    const ti_c54x_arch_state *state,
    const ti_c54x_address_result *result,
    ti_c54x_address_update_phase requested_phase,
    ti_c54x_sequencer_address_commit *out_commit)
{
    ti_c54x_sequencer_address_commit candidate;
    const ti_c54x_address_effect *const effect =
        result == NULL ? NULL : &result->effect;
    ti_c54x_address_apply_status status;

    if (out_commit != NULL) {
        (void)memset(out_commit, 0, sizeof(*out_commit));
    }
    if (state == NULL || result == NULL || out_commit == NULL ||
        requested_phase < TI_C54X_ADDRESS_UPDATE_PHASE_NONE ||
        requested_phase > TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) {
        return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
    }
    if (result->status != TI_C54X_ADDRESS_STATUS_OK) {
        return TI_C54X_ADDRESS_APPLY_REJECTED_RESULT;
    }

    (void)memset(&candidate, 0, sizeof(candidate));
    status = ti_c54x_sequencer_validate_address_effect_intrinsic(
        effect, requested_phase, &candidate);
    if (status != TI_C54X_ADDRESS_APPLY_OK) {
        return status;
    }
    status = ti_c54x_sequencer_validate_address_effect_dependencies(
        state, effect);
    if (status != TI_C54X_ADDRESS_APPLY_OK) {
        return status;
    }

    *out_commit = candidate;
    return TI_C54X_ADDRESS_APPLY_OK;
}
