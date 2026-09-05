/*
 * Module contract
 *
 * Owns: ordered validation and transactional application of one indirect
 * address effect to caller-owned known auxiliary-register state.
 * Does not own: address planning arithmetic, opcode decoding, CPU lifecycle,
 * bus transfers, memory, timing, ARP/CMPT policy, diagnostics, or public ABI.
 */

#include "address_effect_apply.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool valid_phase(ti_c54x_address_update_phase phase)
{
    return phase >= TI_C54X_ADDRESS_UPDATE_PHASE_NONE &&
           phase <= TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS;
}

static bool valid_ar_index(uint8_t index)
{
    return index <= UINT8_C(7);
}

ti_c54x_address_apply_status
ti_c54x_sequencer_apply_address_result(
    ti_c54x_arch_state *state,
    const ti_c54x_address_result *result,
    ti_c54x_address_update_phase requested_phase)
{
    const ti_c54x_address_effect *const effect =
        result == NULL ? NULL : &result->effect;
    const ti_c54x_address_update *const update =
        effect == NULL ? NULL : &effect->update;
    uint16_t current_ar;
    ti_c54x_arch_state_status read_status;

    if (state == NULL || result == NULL || !valid_phase(requested_phase)) {
        return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
    }
    if (result->status != TI_C54X_ADDRESS_STATUS_OK) {
        return TI_C54X_ADDRESS_APPLY_REJECTED_RESULT;
    }

    if (!valid_ar_index(effect->selected_ar_index)) {
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }
    if (update->target == TI_C54X_ADDRESS_UPDATE_NONE) {
        if (update->phase != TI_C54X_ADDRESS_UPDATE_PHASE_NONE ||
            update->ar_index != UINT8_C(0) ||
            update->value_before != UINT16_C(0) ||
            update->value_after != UINT16_C(0)) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
    } else if (update->target == TI_C54X_ADDRESS_UPDATE_AR) {
        if ((update->phase != TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS &&
             update->phase != TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ||
            !valid_ar_index(update->ar_index) ||
            update->ar_index != effect->selected_ar_index) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
    } else {
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }

    if (requested_phase != update->phase) {
        return TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH;
    }

    if (update->target == TI_C54X_ADDRESS_UPDATE_AR) {
        if ((update->phase == TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS &&
             effect->effective_address_word != update->value_after) ||
            (update->phase == TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
             effect->effective_address_word != update->value_before)) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
    }

    read_status = ti_c54x_arch_state_read_ar(
        state, effect->selected_ar_index, &current_ar);
    if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
        return TI_C54X_ADDRESS_APPLY_UNKNOWN_AR;
    }
    if (read_status != TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
    }

    if (update->target == TI_C54X_ADDRESS_UPDATE_NONE) {
        if (current_ar != effect->effective_address_word) {
            return TI_C54X_ADDRESS_APPLY_STALE_AR;
        }
        return TI_C54X_ADDRESS_APPLY_OK;
    }

    if (current_ar != update->value_before) {
        return TI_C54X_ADDRESS_APPLY_STALE_AR;
    }
    if (ti_c54x_arch_state_write_ar(
            state, update->ar_index, update->value_after) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
    }
    return TI_C54X_ADDRESS_APPLY_OK;
}
