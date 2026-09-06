/*
 * Module contract
 *
 * Owns: checked CMPT=0 WORD16 reverse-carry MOD4/MOD7 address effects and
 * their intrinsic shape validator.
 * Does not own: CPU/register storage, ARP/CMPT state, register mutation,
 * memory or bus transfers, opcode decoding, LONG32/circular addressing,
 * timing, or diagnostics.
 */

#include "indirect_reverse_carry.h"

#include "../foundation/reverse_carry.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_address_result
ti_c54x_reverse_carry_result_with_status(ti_c54x_address_status status)
{
    ti_c54x_address_result result;
    (void)memset(&result, 0, sizeof(result));
    result.status = status;
    return result;
}

static bool
ti_c54x_reverse_carry_mode_valid(
    ti_c54x_indirect_reverse_carry_mode mode)
{
    return mode == TI_C54X_INDIRECT_REVERSE_CARRY_MOD4 ||
           mode == TI_C54X_INDIRECT_REVERSE_CARRY_MOD7;
}

static bool
ti_c54x_reverse_carry_access_valid(ti_c54x_address_access access)
{
    return access == TI_C54X_ADDRESS_ACCESS_READ ||
           access == TI_C54X_ADDRESS_ACCESS_WRITE;
}

static bool
ti_c54x_reverse_carry_ar0_is_one_hot(uint16_t ar0_value)
{
    return ar0_value != UINT16_C(0) &&
           (ar0_value & (uint16_t)(ar0_value - UINT16_C(1))) ==
               UINT16_C(0);
}

static ti_c54x_reverse_carry_result
ti_c54x_reverse_carry_update(uint16_t ar_value,
                             uint16_t ar0_value,
                             ti_c54x_indirect_reverse_carry_mode mode)
{
    if (mode == TI_C54X_INDIRECT_REVERSE_CARRY_MOD4) {
        return ti_c54x_reverse_carry_subtract16(ar_value, ar0_value);
    }
    return ti_c54x_reverse_carry_add16(ar_value, ar0_value);
}

ti_c54x_address_result
ti_c54x_indirect_reverse_carry_word16(
    uint8_t ar_index,
    uint16_t ar_value,
    uint16_t ar0_value,
    ti_c54x_indirect_reverse_carry_mode mode,
    ti_c54x_address_access access)
{
    ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {.operand = {0}}
    };
    ti_c54x_reverse_carry_result update_result;

    /* Discriminants are checked before mode-specific sources and aliasing. */
    if (ar_index > UINT8_C(7) || !ti_c54x_reverse_carry_mode_valid(mode) ||
        !ti_c54x_reverse_carry_access_valid(access)) {
        return ti_c54x_reverse_carry_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    if (!ti_c54x_reverse_carry_ar0_is_one_hot(ar0_value)) {
        return ti_c54x_reverse_carry_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    /* The syntax is architecturally legal, but update/source ordering for
     * selected AR0 is outside this slice. */
    if (ar_index == UINT8_C(0)) {
        return ti_c54x_reverse_carry_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
    }

    update_result = ti_c54x_reverse_carry_update(ar_value, ar0_value, mode);
    if (update_result.status != TI_C54X_REVERSE_CARRY_STATUS_OK) {
        return ti_c54x_reverse_carry_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    }

    result.effect.operand = ti_c54x_data_operand_word16(ar_value);
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY;
    result.effect.source.value.ar_reverse_carry.index = ar_index;
    result.effect.source.value.ar_reverse_carry.modification =
        (uint8_t)mode;
    result.effect.source.value.ar_reverse_carry.value_before = ar_value;
    result.effect.source.value.ar_reverse_carry.ar0_value_before = ar0_value;
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0;
    result.effect.update.target = TI_C54X_ADDRESS_UPDATE_AR;
    result.effect.update.phase = TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS;
    result.effect.update.ar_index = ar_index;
    result.effect.update.value_before = ar_value;
    result.effect.update.value_after = update_result.value;
    return result;
}

bool
ti_c54x_indirect_reverse_carry_effect_is_valid(
    const ti_c54x_address_effect *effect)
{
    const ti_c54x_address_source_ar_reverse_carry *source;
    ti_c54x_indirect_reverse_carry_mode mode;
    ti_c54x_reverse_carry_result expected;

    if (effect == NULL || !ti_c54x_data_operand_is_valid(&effect->operand) ||
        effect->operand.width != TI_C54X_DATA_OPERAND_WIDTH_WORD16 ||
        effect->source.kind != TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY ||
        effect->state_policy != TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) {
        return false;
    }

    source = &effect->source.value.ar_reverse_carry;
    if (source->index == UINT8_C(0) || source->index > UINT8_C(7) ||
        !ti_c54x_reverse_carry_ar0_is_one_hot(source->ar0_value_before) ||
        (source->modification !=
             (uint8_t)TI_C54X_INDIRECT_REVERSE_CARRY_MOD4 &&
         source->modification !=
             (uint8_t)TI_C54X_INDIRECT_REVERSE_CARRY_MOD7) ||
        effect->update.target != TI_C54X_ADDRESS_UPDATE_AR ||
        effect->update.phase != TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS ||
        effect->update.ar_index != source->index ||
        effect->update.value_before != source->value_before ||
        effect->operand.first_word != source->value_before) {
        return false;
    }

    mode = (ti_c54x_indirect_reverse_carry_mode)source->modification;
    expected = ti_c54x_reverse_carry_update(
        source->value_before, source->ar0_value_before, mode);
    return expected.status == TI_C54X_REVERSE_CARRY_STATUS_OK &&
           effect->update.value_after == expected.value;
}
