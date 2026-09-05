/*
 * Module contract
 *
 * Owns: ordered validation and transactional application of one tagged
 * address effect to caller-owned architectural state. Intrinsic operand and
 * source shape are validated before phase, mode-state, or source reads.
 * Does not own: address planning arithmetic, opcode decoding, CPU lifecycle,
 * bus transfers, memory, timing, diagnostics, or public ABI. It validates the
 * narrow state precondition carried by an effect without repairing state.
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

static bool valid_source_kind(ti_c54x_address_source_kind kind)
{
    return kind >= TI_C54X_ADDRESS_SOURCE_AR &&
           kind <= TI_C54X_ADDRESS_SOURCE_DIRECT_SP;
}

static bool valid_state_policy_for_source(
    ti_c54x_address_source_kind source_kind,
    ti_c54x_address_state_policy policy)
{
    return (source_kind == TI_C54X_ADDRESS_SOURCE_AR &&
            (policy == TI_C54X_ADDRESS_STATE_POLICY_NONE ||
             policy == TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0)) ||
           (source_kind == TI_C54X_ADDRESS_SOURCE_DIRECT_DP &&
            policy == TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0) ||
           (source_kind == TI_C54X_ADDRESS_SOURCE_DIRECT_SP &&
            policy == TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL1);
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

    if (!ti_c54x_data_operand_is_valid(&effect->operand)) {
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }

    if (!valid_source_kind(effect->source.kind)) {
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }
    switch (effect->source.kind) {
    case TI_C54X_ADDRESS_SOURCE_AR:
        if (!valid_ar_index(effect->source.value.ar.index)) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
        break;
    case TI_C54X_ADDRESS_SOURCE_DIRECT_DP:
        if (effect->source.value.direct_dp.dp9 > UINT16_C(0x01ff) ||
            effect->source.value.direct_dp.dma7 > UINT8_C(0x7f) ||
            effect->operand.width != TI_C54X_DATA_OPERAND_WIDTH_WORD16 ||
            effect->operand.first_word !=
                (uint16_t)((effect->source.value.direct_dp.dp9 << 7U) |
                           effect->source.value.direct_dp.dma7)) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
        break;
    case TI_C54X_ADDRESS_SOURCE_DIRECT_SP: {
        const uint32_t sum =
            (uint32_t)effect->source.value.direct_sp.sp16 +
            (uint32_t)effect->source.value.direct_sp.dma7;
        if (effect->source.value.direct_sp.dma7 > UINT8_C(0x7f) ||
            sum > UINT32_C(0xffff) ||
            effect->operand.width != TI_C54X_DATA_OPERAND_WIDTH_WORD16 ||
            effect->operand.first_word != (uint16_t)sum) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
        break;
    }
    default:
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }

    if (update->target == TI_C54X_ADDRESS_UPDATE_NONE) {
        if (update->phase != TI_C54X_ADDRESS_UPDATE_PHASE_NONE ||
            update->ar_index != UINT8_C(0) ||
            update->value_before != UINT16_C(0) ||
            update->value_after != UINT16_C(0)) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
        if (effect->source.kind == TI_C54X_ADDRESS_SOURCE_AR &&
            effect->operand.first_word != effect->source.value.ar.value_before) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
    } else if (update->target == TI_C54X_ADDRESS_UPDATE_AR) {
        if ((update->phase != TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS &&
             update->phase != TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ||
            !valid_ar_index(update->ar_index) ||
            effect->source.kind != TI_C54X_ADDRESS_SOURCE_AR ||
            update->ar_index != effect->source.value.ar.index ||
            update->value_before != effect->source.value.ar.value_before) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
    } else {
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }

    if (!valid_state_policy_for_source(effect->source.kind,
                                       effect->state_policy)) {
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }

    if (requested_phase != update->phase) {
        return TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH;
    }

    if (update->target == TI_C54X_ADDRESS_UPDATE_AR) {
        if ((update->phase == TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS &&
             effect->operand.first_word != update->value_after) ||
            (update->phase == TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
             effect->operand.first_word != update->value_before)) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
    }

    if (effect->state_policy == TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) {
        uint8_t cmpt = UINT8_C(0);
        uint8_t arp = UINT8_C(0);
        const ti_c54x_arch_state_status cmpt_status =
            ti_c54x_arch_state_read_cmpt(state, &cmpt);
        const ti_c54x_arch_state_status arp_status =
            ti_c54x_arch_state_read_arp(state, &arp);

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
        return TI_C54X_ADDRESS_APPLY_OK;
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
        return TI_C54X_ADDRESS_APPLY_OK;
    }
    default:
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }

    if (update->target == TI_C54X_ADDRESS_UPDATE_NONE) {
        return TI_C54X_ADDRESS_APPLY_OK;
    }

    if (ti_c54x_arch_state_write_ar(
            state, effect->source.value.ar.index, update->value_after) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
    }
    return TI_C54X_ADDRESS_APPLY_OK;
}
