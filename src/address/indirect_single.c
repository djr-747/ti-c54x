/*
 * Module contract
 *
 * Owns: checked CMPT=0 single-indirect word effective-address planning for
 * MOD0 through MOD3, including ordered auxiliary-register effects.
 * Does not own: CPU state, ARP/CMPT state validation, bus transfers, opcode
 * decoding, long operands, circular or bit-reversed addressing, dual-indirect
 * forms, instruction scheduling, or diagnostics.
 */

#include "indirect_single.h"

#include <stdbool.h>
#include <stdint.h>

static ti_c54x_address_result
ti_c54x_address_result_with_status(ti_c54x_address_status status)
{
    const ti_c54x_address_result result = {
        .status = status,
        .effect = {0}
    };
    return result;
}

static bool
ti_c54x_indirect_mode_valid(ti_c54x_indirect_single_mode mode)
{
    return mode >= TI_C54X_INDIRECT_SINGLE_MOD0 &&
           mode <= TI_C54X_INDIRECT_SINGLE_MOD3;
}

static bool
ti_c54x_address_access_valid(ti_c54x_address_access access)
{
    return access >= TI_C54X_ADDRESS_ACCESS_READ &&
           access <= TI_C54X_ADDRESS_ACCESS_WRITE;
}

static ti_c54x_address_update
ti_c54x_ar_update(ti_c54x_address_update_phase phase,
                  uint8_t ar_index,
                  uint16_t value_before,
                  uint16_t value_after)
{
    const ti_c54x_address_update update = {
        .target = TI_C54X_ADDRESS_UPDATE_AR,
        .phase = phase,
        .ar_index = ar_index,
        .value_before = value_before,
        .value_after = value_after
    };
    return update;
}

ti_c54x_address_result
ti_c54x_indirect_single_word(uint8_t ar_index,
                             uint16_t ar_value,
                             ti_c54x_indirect_single_mode mode,
                             ti_c54x_address_access access)
{
    ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {0}
    };

    if (ar_index > UINT8_C(7) || !ti_c54x_indirect_mode_valid(mode) ||
        !ti_c54x_address_access_valid(access)) {
        return ti_c54x_address_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    if (mode == TI_C54X_INDIRECT_SINGLE_MOD3 &&
        access == TI_C54X_ADDRESS_ACCESS_READ) {
        return ti_c54x_address_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
    }
    if ((mode == TI_C54X_INDIRECT_SINGLE_MOD1 && ar_value == UINT16_C(0)) ||
        ((mode == TI_C54X_INDIRECT_SINGLE_MOD2 ||
          mode == TI_C54X_INDIRECT_SINGLE_MOD3) &&
         ar_value == UINT16_MAX)) {
        return ti_c54x_address_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    }

    result.status = TI_C54X_ADDRESS_STATUS_OK;
    result.effect.effective_address_word = ar_value;
    result.effect.selected_ar_index = ar_index;

    switch (mode) {
    case TI_C54X_INDIRECT_SINGLE_MOD0:
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD1:
        result.effect.update = ti_c54x_ar_update(
            TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
            ar_index,
            ar_value,
            (uint16_t)(ar_value - UINT16_C(1)));
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD2:
        result.effect.update = ti_c54x_ar_update(
            TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
            ar_index,
            ar_value,
            (uint16_t)(ar_value + UINT16_C(1)));
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD3:
        result.effect.effective_address_word =
            (ti_c54x_data_word_address)(ar_value + UINT16_C(1));
        result.effect.update = ti_c54x_ar_update(
            TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
            ar_index,
            ar_value,
            (uint16_t)(ar_value + UINT16_C(1)));
        break;
    default:
        return ti_c54x_address_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }

    return result;
}
