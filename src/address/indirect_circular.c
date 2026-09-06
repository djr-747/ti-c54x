/*
 * Module contract
 *
 * Owns: checked CMPT=0 fixed-step circular WORD16 planning for MOD8/MOD10
 * and intrinsic validation of its value-owned effect shape.
 * Does not own: CPU state, ARP/CMPT/BK dependency validation, architectural
 * mutation, memory or bus transfers, opcode decoding, sequencing, timing, or
 * diagnostics.
 */

#include "indirect_circular.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_address_result
ti_c54x_indirect_circular_result_with_status(ti_c54x_address_status status)
{
    ti_c54x_address_result result;

    (void)memset(&result, 0, sizeof(result));
    result.status = status;
    return result;
}

static bool
ti_c54x_indirect_circular_mode_is_valid(
    ti_c54x_indirect_circular_mode mode)
{
    return mode == TI_C54X_INDIRECT_CIRCULAR_MOD8 ||
           mode == TI_C54X_INDIRECT_CIRCULAR_MOD10;
}

static bool
ti_c54x_indirect_circular_access_is_valid(ti_c54x_address_access access)
{
    return access == TI_C54X_ADDRESS_ACCESS_READ ||
           access == TI_C54X_ADDRESS_ACCESS_WRITE;
}

/* Return the least power-of-two EFB size strictly greater than BK. BK is
 * 16-bit, so the largest result is 2^16 and uint32_t is sufficient. */
static uint32_t
ti_c54x_indirect_circular_efb_size(uint16_t bk_value)
{
    uint32_t efb_size = UINT32_C(1);

    while (efb_size <= (uint32_t)bk_value) {
        efb_size <<= 1U;
    }
    return efb_size;
}

/* Compute post-access fixed-step circular movement. The caller has already
 * established BK>0 and that the initial index is strictly below BK. */
static bool
ti_c54x_indirect_circular_after(uint16_t ar_value,
                                uint16_t bk_value,
                                ti_c54x_indirect_circular_mode mode,
                                uint16_t *value_after)
{
    const uint32_t efb_size =
        ti_c54x_indirect_circular_efb_size(bk_value);
    const uint32_t efb_mask = efb_size - UINT32_C(1);
    const uint32_t ar32 = (uint32_t)ar_value;
    const uint32_t index = ar32 & efb_mask;
    const int32_t step = mode == TI_C54X_INDIRECT_CIRCULAR_MOD8
                             ? INT32_C(-1)
                             : INT32_C(1);
    int32_t moved = (int32_t)index + step;
    uint32_t result;

    if (index >= (uint32_t)bk_value) {
        return false;
    }
    if (moved < INT32_C(0)) {
        moved += (int32_t)bk_value;
    } else if ((uint32_t)moved >= (uint32_t)bk_value) {
        moved -= (int32_t)bk_value;
    }
    result = (ar32 & ~efb_mask) + (uint32_t)moved;
    if (result > UINT32_C(0xffff)) {
        return false;
    }
    *value_after = (uint16_t)result;
    return true;
}

static bool
ti_c54x_indirect_circular_update_is_none(
    const ti_c54x_address_update *update)
{
    return update->target == TI_C54X_ADDRESS_UPDATE_NONE &&
           update->phase == TI_C54X_ADDRESS_UPDATE_PHASE_NONE &&
           update->ar_index == UINT8_C(0) &&
           update->value_before == UINT16_C(0) &&
           update->value_after == UINT16_C(0);
}

ti_c54x_address_result
ti_c54x_indirect_circular_word16(uint8_t ar_index,
                                 uint16_t ar_value,
                                 uint16_t bk_value,
                                 ti_c54x_indirect_circular_mode mode,
                                 ti_c54x_address_access access)
{
    ti_c54x_address_result result;
    uint16_t value_after;

    if (ar_index > UINT8_C(7) ||
        !ti_c54x_indirect_circular_mode_is_valid(mode) ||
        !ti_c54x_indirect_circular_access_is_valid(access)) {
        return ti_c54x_indirect_circular_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }

    (void)memset(&result, 0, sizeof(result));
    result.status = TI_C54X_ADDRESS_STATUS_OK;
    result.effect.operand = ti_c54x_data_operand_word16(ar_value);
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR;
    result.effect.source.value.ar_circular.index = ar_index;
    result.effect.source.value.ar_circular.modification = (uint8_t)mode;
    result.effect.source.value.ar_circular.value_before = ar_value;
    result.effect.source.value.ar_circular.bk_value_before = bk_value;
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0;

    if (bk_value == UINT16_C(0)) {
        return result;
    }
    if (!ti_c54x_indirect_circular_after(ar_value, bk_value, mode,
                                         &value_after)) {
        return ti_c54x_indirect_circular_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    }

    result.effect.update.target = TI_C54X_ADDRESS_UPDATE_AR;
    result.effect.update.phase = TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS;
    result.effect.update.ar_index = ar_index;
    result.effect.update.value_before = ar_value;
    result.effect.update.value_after = value_after;
    return result;
}

bool
ti_c54x_indirect_circular_effect_is_valid(
    const ti_c54x_address_effect *effect)
{
    const ti_c54x_address_source_ar_circular *source;
    uint16_t expected_after;

    if (effect == NULL ||
        !ti_c54x_data_operand_is_valid(&effect->operand) ||
        effect->operand.width != TI_C54X_DATA_OPERAND_WIDTH_WORD16 ||
        effect->source.kind != TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR ||
        effect->state_policy != TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) {
        return false;
    }
    source = &effect->source.value.ar_circular;
    if (source->index > UINT8_C(7) ||
        !ti_c54x_indirect_circular_mode_is_valid(
            (ti_c54x_indirect_circular_mode)source->modification) ||
        effect->operand.first_word != source->value_before) {
        return false;
    }
    if (source->bk_value_before == UINT16_C(0)) {
        return ti_c54x_indirect_circular_update_is_none(&effect->update);
    }
    if (!ti_c54x_indirect_circular_after(
            source->value_before, source->bk_value_before,
            (ti_c54x_indirect_circular_mode)source->modification,
            &expected_after)) {
        return false;
    }
    return effect->update.target == TI_C54X_ADDRESS_UPDATE_AR &&
           effect->update.phase == TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
           effect->update.ar_index == source->index &&
           effect->update.value_before == source->value_before &&
           effect->update.value_after == expected_after;
}
