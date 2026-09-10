/*
 * Module contract
 *
 * Owns: state-free validation of one borrowed address effect, including
 * address-owned validators, operand/source shape, update shape, policy,
 * phase, and operand/update relations. It produces the private selected-AR
 * commit description consumed by the sequencer.
 * Does not own: architectural state reads or writes, address planning,
 * opcode decoding, CPU lifecycle, buses, memory, timing, diagnostics, or
 * public ABI. Circular MOD8/MOD10 WORD16 effects are delegated to the
 * address-owned circular validator before generic shape checks; BK=0's
 * canonical NONE update is accepted only through that validator. The effect
 * is borrowed for the duration of the call and is never retained or modified;
 * the output is caller-owned and zeroed before validation.
 */

#include "address_effect_intrinsic.h"

#include "address/indirect_circular.h"
#include "address/indirect_long_offset.h"
#include "address/indirect_reverse_carry.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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
    switch (kind) {
    case TI_C54X_ADDRESS_SOURCE_AR:
    case TI_C54X_ADDRESS_SOURCE_DIRECT_DP:
    case TI_C54X_ADDRESS_SOURCE_DIRECT_SP:
    case TI_C54X_ADDRESS_SOURCE_AR_INDEXED:
    case TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET:
    case TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY:
    case TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR:
    case TI_C54X_ADDRESS_SOURCE_STACK_PUSH:
        return true;
    default:
        return false;
    }
}

static bool valid_state_policy_for_source(
    ti_c54x_address_source_kind source_kind,
    ti_c54x_address_state_policy policy)
{
    return (source_kind == TI_C54X_ADDRESS_SOURCE_AR &&
            (policy == TI_C54X_ADDRESS_STATE_POLICY_NONE ||
             policy == TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0)) ||
           (source_kind == TI_C54X_ADDRESS_SOURCE_AR_INDEXED &&
            policy == TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) ||
           (source_kind == TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET &&
            policy == TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) ||
           (source_kind == TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY &&
            policy == TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) ||
           (source_kind == TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR &&
            policy == TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) ||
           (source_kind == TI_C54X_ADDRESS_SOURCE_DIRECT_DP &&
            policy == TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0) ||
           (source_kind == TI_C54X_ADDRESS_SOURCE_DIRECT_SP &&
            policy == TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL1) ||
           (source_kind == TI_C54X_ADDRESS_SOURCE_STACK_PUSH &&
            policy == TI_C54X_ADDRESS_STATE_POLICY_NONE);
}

static bool source_supports_ar_update(ti_c54x_address_source_kind source_kind)
{
    return source_kind == TI_C54X_ADDRESS_SOURCE_AR ||
           source_kind == TI_C54X_ADDRESS_SOURCE_AR_INDEXED ||
           source_kind == TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET ||
           source_kind == TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY ||
           source_kind == TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR;
}

static uint8_t source_ar_index(const ti_c54x_address_source *source)
{
    switch (source->kind) {
    case TI_C54X_ADDRESS_SOURCE_AR:
        return source->value.ar.index;
    case TI_C54X_ADDRESS_SOURCE_AR_INDEXED:
        return source->value.ar_indexed.index;
    case TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET:
        return source->value.ar_long_offset.index;
    case TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY:
        return source->value.ar_reverse_carry.index;
    case TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR:
        return source->value.ar_circular.index;
    default:
        return UINT8_C(0);
    }
}

static uint16_t source_ar_value_before(const ti_c54x_address_source *source)
{
    switch (source->kind) {
    case TI_C54X_ADDRESS_SOURCE_AR:
        return source->value.ar.value_before;
    case TI_C54X_ADDRESS_SOURCE_AR_INDEXED:
        return source->value.ar_indexed.value_before;
    case TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET:
        return source->value.ar_long_offset.value_before;
    case TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY:
        return source->value.ar_reverse_carry.value_before;
    case TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR:
        return source->value.ar_circular.value_before;
    default:
        return UINT16_C(0);
    }
}

ti_c54x_address_apply_status
ti_c54x_sequencer_validate_address_effect_intrinsic(
    const ti_c54x_address_effect *effect,
    ti_c54x_address_update_phase requested_phase,
    ti_c54x_sequencer_address_commit *out_commit)
{
    const ti_c54x_address_update *const update =
        effect == NULL ? NULL : &effect->update;

    if (out_commit != NULL) {
        (void)memset(out_commit, 0, sizeof(*out_commit));
    }
    if (effect == NULL || out_commit == NULL || !valid_phase(requested_phase)) {
        return TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT;
    }

    /* Address-owned validators run before the generic operand validator. */
    if (effect->source.kind == TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET &&
        !ti_c54x_indirect_long_offset_effect_is_valid(effect)) {
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }
    if (effect->source.kind == TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY &&
        !ti_c54x_indirect_reverse_carry_effect_is_valid(effect)) {
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }
    if (effect->source.kind == TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR &&
        !ti_c54x_indirect_circular_effect_is_valid(effect)) {
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
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
    case TI_C54X_ADDRESS_SOURCE_AR_INDEXED:
        if (effect->source.value.ar_indexed.index == UINT8_C(0) ||
            effect->source.value.ar_indexed.index > UINT8_C(7) ||
            effect->operand.width != TI_C54X_DATA_OPERAND_WIDTH_WORD16 ||
            effect->operand.first_word !=
                effect->source.value.ar_indexed.value_before ||
            effect->update.target != TI_C54X_ADDRESS_UPDATE_AR ||
            effect->update.phase !=
                TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS ||
            effect->update.ar_index != effect->source.value.ar_indexed.index ||
            effect->update.value_before !=
                effect->source.value.ar_indexed.value_before) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
        break;
    case TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET:
    case TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY:
    case TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR:
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
    case TI_C54X_ADDRESS_SOURCE_STACK_PUSH:
        /* Every 16-bit snapshot is in-domain; the TOS wrap arithmetic is
         * proven by the root candidate check, mirroring AR-indirect
         * layering. Only the WORD16 shape is intrinsic here. */
        if (effect->operand.width != TI_C54X_DATA_OPERAND_WIDTH_WORD16) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
        break;
    default:
        return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
    }

    /* NONE and AR0-indexed update discriminators are intrinsic shape. */
    if (update->target == TI_C54X_ADDRESS_UPDATE_NONE) {
        if (update->phase != TI_C54X_ADDRESS_UPDATE_PHASE_NONE ||
            update->ar_index != UINT8_C(0) ||
            update->value_before != UINT16_C(0) ||
            update->value_after != UINT16_C(0)) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
        if ((effect->source.kind == TI_C54X_ADDRESS_SOURCE_AR &&
             effect->operand.first_word !=
                 effect->source.value.ar.value_before) ||
            (effect->source.kind == TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR &&
             effect->source.value.ar_circular.bk_value_before !=
                 UINT16_C(0))) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
    } else if (update->target == TI_C54X_ADDRESS_UPDATE_AR) {
        if ((update->phase != TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS &&
             update->phase != TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ||
            !valid_ar_index(update->ar_index) ||
            !source_supports_ar_update(effect->source.kind) ||
            update->ar_index != source_ar_index(&effect->source) ||
            update->value_before != source_ar_value_before(&effect->source)) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
    } else if (update->target == TI_C54X_ADDRESS_UPDATE_SP) {
        if (update->ar_index != UINT8_C(0) ||
            effect->source.kind != TI_C54X_ADDRESS_SOURCE_STACK_PUSH ||
            update->value_before !=
                effect->source.value.stack_push.sp_before ||
            (update->phase != TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS &&
             update->phase != TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS)) {
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
    if (update->target == TI_C54X_ADDRESS_UPDATE_SP) {
        /* Pushes address the post-update register (MOD3 PRE shape);
         * pops address the pre-update register (MOD0 POST shape). */
        if ((update->phase == TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS &&
             effect->operand.first_word != update->value_after) ||
            (update->phase == TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
             effect->operand.first_word != update->value_before)) {
            return TI_C54X_ADDRESS_APPLY_INVALID_EFFECT;
        }
    }

    if (update->target == TI_C54X_ADDRESS_UPDATE_AR) {
        out_commit->target = TI_C54X_ADDRESS_UPDATE_AR;
        out_commit->ar_index = update->ar_index;
        out_commit->value_after = update->value_after;
    }
    if (update->target == TI_C54X_ADDRESS_UPDATE_SP) {
        out_commit->target = TI_C54X_ADDRESS_UPDATE_SP;
        out_commit->ar_index = UINT8_C(0);
        out_commit->value_after = update->value_after;
    }
    return TI_C54X_ADDRESS_APPLY_OK;
}
