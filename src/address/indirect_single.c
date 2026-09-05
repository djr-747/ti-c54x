/*
 * Module contract
 *
 * Owns: checked CMPT=0 single-indirect word and long32 effective-address
 * planning for MOD0 through MOD3, including ordered auxiliary-register
 * effects.
 * Does not own: CPU state or ARP/CMPT state validation, bus transfers, opcode
 * decoding, circular or bit-reversed addressing, dual-indirect forms,
 * instruction scheduling, or diagnostics.
 */

#include "indirect_single.h"

#include <stdbool.h>
#include <stdint.h>

static ti_c54x_address_result
ti_c54x_address_result_with_status(ti_c54x_address_status status)
{
    const ti_c54x_address_result result = {
        .status = status,
        .effect = {.operand = {0}}
    };
    return result;
}

static bool
ti_c54x_update_crosses_unresolved_boundary(
    ti_c54x_data_operand_width width,
    ti_c54x_indirect_single_mode mode,
    uint16_t ar_value)
{
    if (width == TI_C54X_DATA_OPERAND_WIDTH_WORD16) {
        return (mode == TI_C54X_INDIRECT_SINGLE_MOD1 &&
                ar_value == UINT16_C(0)) ||
               ((mode == TI_C54X_INDIRECT_SINGLE_MOD2 ||
                 mode == TI_C54X_INDIRECT_SINGLE_MOD3) &&
                ar_value == UINT16_MAX);
    }
    return (mode == TI_C54X_INDIRECT_SINGLE_MOD1 && ar_value < UINT16_C(2)) ||
           ((mode == TI_C54X_INDIRECT_SINGLE_MOD2 ||
             mode == TI_C54X_INDIRECT_SINGLE_MOD3) &&
            ar_value > UINT16_C(0xfffd));
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

static ti_c54x_address_result
ti_c54x_indirect_single_plan(uint8_t ar_index,
                             uint16_t ar_value,
                             ti_c54x_indirect_single_mode mode,
                             ti_c54x_address_access access,
                             ti_c54x_data_operand_width width)
{
    ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {.operand = {0}}
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
    if (ti_c54x_update_crosses_unresolved_boundary(width, mode, ar_value)) {
        return ti_c54x_address_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    }

    result.status = TI_C54X_ADDRESS_STATUS_OK;
    result.effect.operand = width == TI_C54X_DATA_OPERAND_WIDTH_WORD16
                                ? ti_c54x_data_operand_word16(ar_value)
                                : ti_c54x_data_operand_long32(ar_value);
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_AR;
    result.effect.source.value.ar.index = ar_index;
    result.effect.source.value.ar.value_before = ar_value;

    switch (mode) {
    case TI_C54X_INDIRECT_SINGLE_MOD0:
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD1:
        result.effect.update = ti_c54x_ar_update(
            TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
            ar_index,
            ar_value,
            (uint16_t)(ar_value -
                       (width == TI_C54X_DATA_OPERAND_WIDTH_LONG32
                            ? UINT16_C(2)
                            : UINT16_C(1))));
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD2:
        result.effect.update = ti_c54x_ar_update(
            TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
            ar_index,
            ar_value,
            (uint16_t)(ar_value +
                       (width == TI_C54X_DATA_OPERAND_WIDTH_LONG32
                            ? UINT16_C(2)
                            : UINT16_C(1))));
        break;
    case TI_C54X_INDIRECT_SINGLE_MOD3:
        result.effect.operand = width == TI_C54X_DATA_OPERAND_WIDTH_WORD16
                                    ? ti_c54x_data_operand_word16(
                                          (uint16_t)(ar_value + UINT16_C(1)))
                                    : ti_c54x_data_operand_long32(
                                          (uint16_t)(ar_value + UINT16_C(2)));
        result.effect.update = ti_c54x_ar_update(
            TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
            ar_index,
            ar_value,
            (uint16_t)(ar_value +
                       (width == TI_C54X_DATA_OPERAND_WIDTH_LONG32
                            ? UINT16_C(2)
                            : UINT16_C(1))));
        break;
    default:
        return ti_c54x_address_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }

    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0;

    return result;
}

ti_c54x_address_result
ti_c54x_indirect_single_word(uint8_t ar_index,
                             uint16_t ar_value,
                             ti_c54x_indirect_single_mode mode,
                             ti_c54x_address_access access)
{
    return ti_c54x_indirect_single_plan(
        ar_index, ar_value, mode, access,
        TI_C54X_DATA_OPERAND_WIDTH_WORD16);
}

ti_c54x_address_result
ti_c54x_indirect_single_long32(uint8_t ar_index,
                               uint16_t ar_value,
                               ti_c54x_indirect_single_mode mode,
                               ti_c54x_address_access access)
{
    return ti_c54x_indirect_single_plan(
        ar_index, ar_value, mode, access,
        TI_C54X_DATA_OPERAND_WIDTH_LONG32);
}
