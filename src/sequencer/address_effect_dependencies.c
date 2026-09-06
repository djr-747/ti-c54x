/*
 * Module contract
 *
 * Owns: ordered, read-only architectural mode and source snapshot checks for
 * an already intrinsically valid borrowed address effect. It reads only the
 * state accessors required by the effect's tagged policy/source and never
 * mutates state.
 * Does not own: effect shape or arithmetic validation, phase/relation policy,
 * commit description construction, architectural writes, planning, decode,
 * CPU lifecycle, buses, memory, timing, diagnostics, or public ABI. State
 * and effect are borrowed for the duration of the call and are not retained.
 * Circular effects read the selected AR and then BK, classify either unknown
 * accessor as UNKNOWN_SOURCE, and never write either register.
 */

#include "address_effect_dependencies.h"

#include "state/block_size_register.h"

#include <stddef.h>
#include <stdint.h>

ti_c54x_address_apply_status
ti_c54x_sequencer_validate_address_effect_dependencies(
    const ti_c54x_arch_state *state,
    const ti_c54x_address_effect *effect)
{
    uint16_t current_ar;
    ti_c54x_arch_state_status read_status;

    if (state == NULL || effect == NULL) {
        return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
    }

    if (effect->state_policy == TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) {
        uint8_t cmpt = UINT8_C(0);
        uint8_t arp = UINT8_C(0);
        const ti_c54x_arch_state_status cmpt_status =
            ti_c54x_arch_state_read_cmpt(state, &cmpt);
        const ti_c54x_arch_state_status arp_status =
            ti_c54x_arch_state_read_arp(state, &arp);

        /* Preserve the CMPT-then-ARP read order and unknown precedence. */
        if (cmpt_status == TI_C54X_ARCH_STATE_UNKNOWN ||
            arp_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE;
        }
        if (cmpt_status != TI_C54X_ARCH_STATE_OK ||
            arp_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (cmpt != UINT8_C(0) || arp != UINT8_C(0)) {
            return TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION;
        }
    }

    switch (effect->source.kind) {
    case TI_C54X_ADDRESS_SOURCE_AR:
        read_status = ti_c54x_arch_state_read_ar(
            state, effect->source.value.ar.index, &current_ar);
        if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE;
        }
        if (read_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (current_ar != effect->source.value.ar.value_before) {
            return TI_C54X_ADDRESS_APPLY_STALE_SOURCE;
        }
        break;
    case TI_C54X_ADDRESS_SOURCE_AR_INDEXED:
        read_status = ti_c54x_arch_state_read_ar(
            state, effect->source.value.ar_indexed.index, &current_ar);
        if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE;
        }
        if (read_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (current_ar != effect->source.value.ar_indexed.value_before) {
            return TI_C54X_ADDRESS_APPLY_STALE_SOURCE;
        }
        read_status = ti_c54x_arch_state_read_ar(state, UINT8_C(0),
                                                 &current_ar);
        if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE;
        }
        if (read_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (current_ar != effect->source.value.ar_indexed.ar0_value_before) {
            return TI_C54X_ADDRESS_APPLY_STALE_SOURCE;
        }
        break;
    case TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET:
        read_status = ti_c54x_arch_state_read_ar(
            state, effect->source.value.ar_long_offset.index, &current_ar);
        if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE;
        }
        if (read_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (current_ar != effect->source.value.ar_long_offset.value_before) {
            return TI_C54X_ADDRESS_APPLY_STALE_SOURCE;
        }
        break;
    case TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY:
        read_status = ti_c54x_arch_state_read_ar(
            state, effect->source.value.ar_reverse_carry.index, &current_ar);
        if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE;
        }
        if (read_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (current_ar != effect->source.value.ar_reverse_carry.value_before) {
            return TI_C54X_ADDRESS_APPLY_STALE_SOURCE;
        }
        read_status = ti_c54x_arch_state_read_ar(state, UINT8_C(0),
                                                 &current_ar);
        if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE;
        }
        if (read_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (current_ar !=
            effect->source.value.ar_reverse_carry.ar0_value_before) {
            return TI_C54X_ADDRESS_APPLY_STALE_SOURCE;
        }
        break;
    case TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR:
        read_status = ti_c54x_arch_state_read_ar(
            state, effect->source.value.ar_circular.index, &current_ar);
        if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE;
        }
        if (read_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (current_ar != effect->source.value.ar_circular.value_before) {
            return TI_C54X_ADDRESS_APPLY_STALE_SOURCE;
        }
        {
            uint16_t current_bk;

            read_status = ti_c54x_arch_state_read_bk(state, &current_bk);
            if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
                return TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE;
            }
            if (read_status != TI_C54X_ARCH_STATE_OK) {
                return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
            }
            if (current_bk !=
                effect->source.value.ar_circular.bk_value_before) {
                return TI_C54X_ADDRESS_APPLY_STALE_SOURCE;
            }
        }
        break;
    case TI_C54X_ADDRESS_SOURCE_DIRECT_DP: {
        uint8_t cpl = UINT8_C(0);
        uint16_t dp = UINT16_C(0);
        read_status = ti_c54x_arch_state_read_cpl(state, &cpl);
        if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE;
        }
        if (read_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (cpl != UINT8_C(0)) {
            return TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION;
        }
        read_status = ti_c54x_arch_state_read_dp(state, &dp);
        if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE;
        }
        if (read_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (dp != effect->source.value.direct_dp.dp9) {
            return TI_C54X_ADDRESS_APPLY_STALE_SOURCE;
        }
        break;
    }
    case TI_C54X_ADDRESS_SOURCE_DIRECT_SP: {
        uint8_t cpl = UINT8_C(0);
        uint16_t sp = UINT16_C(0);
        read_status = ti_c54x_arch_state_read_cpl(state, &cpl);
        if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE;
        }
        if (read_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (cpl != UINT8_C(1)) {
            return TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION;
        }
        read_status = ti_c54x_arch_state_read_sp(state, &sp);
        if (read_status == TI_C54X_ARCH_STATE_UNKNOWN) {
            return TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE;
        }
        if (read_status != TI_C54X_ARCH_STATE_OK) {
            return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
        }
        if (sp != effect->source.value.direct_sp.sp16) {
            return TI_C54X_ADDRESS_APPLY_STALE_SOURCE;
        }
        break;
    }
    default:
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }

    return TI_C54X_ADDRESS_APPLY_OK;
}
