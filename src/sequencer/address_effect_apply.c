/*
 * Module contract
 *
 * Owns: the final transactional application of a validated address effect to
 * caller-owned architectural state. Validation and all dependency reads are
 * delegated to address_effect_validate.c; this module performs only the sole
 * selected-AR write described by its private commit descriptor.
 * Does not own: address planning arithmetic, opcode decoding, CPU lifecycle,
 * bus transfers, memory, timing, diagnostics, or public ABI.
 */

#include "address_effect_apply.h"
#include "address_effect_validate.h"

#include <stdint.h>

ti_c54x_address_apply_status
ti_c54x_sequencer_apply_address_result(
    ti_c54x_arch_state *state,
    const ti_c54x_address_result *result,
    ti_c54x_address_update_phase requested_phase)
{
    ti_c54x_sequencer_address_commit commit;
    const ti_c54x_address_apply_status status =
        ti_c54x_sequencer_validate_address_effect(
            state, result, requested_phase, &commit);

    if (status != TI_C54X_ADDRESS_APPLY_OK) {
        return status;
    }
    if (commit.target == TI_C54X_ADDRESS_UPDATE_NONE) {
        return TI_C54X_ADDRESS_APPLY_OK;
    }
    if (commit.target == TI_C54X_ADDRESS_UPDATE_SP) {
        if (ti_c54x_arch_state_write_sp(state, commit.value_after) !=
            TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        return TI_C54X_ADDRESS_APPLY_OK;
    }
    if (commit.target != TI_C54X_ADDRESS_UPDATE_AR ||
        ti_c54x_arch_state_write_ar(state, commit.ar_index,
                                    commit.value_after) !=
            TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
    }
    return TI_C54X_ADDRESS_APPLY_OK;
}
