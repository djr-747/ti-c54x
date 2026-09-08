/*
 * Module contract
 *
 * Owns: checked W51A dual-indirect MOD0/MOD1/MOD2 WORD16 effective-address
 * planning, including copied X/Y auxiliary-register snapshots, post-access
 * updates, and transactional rejection of unsupported or inconsistent input.
 * Does not own: CPU/register storage, ARP/CMPT validation, register mutation,
 * bus transfers, opcode decoding, instruction sequencing, timing, or
 * diagnostics. No pointer, allocator, process state, or I/O is used.
 */

#include "indirect_dual.h"

#include <stdbool.h>

static ti_c54x_address_result
ti_c54x_dual_operand_result(ti_c54x_address_status status)
{
    const ti_c54x_address_result result = {
        .status = status,
        .effect = {.operand = {0}}
    };
    return result;
}

static ti_c54x_indirect_dual_result
ti_c54x_dual_result_with_status(ti_c54x_address_status status)
{
    const ti_c54x_indirect_dual_result result = {
        .status = status,
        .x = ti_c54x_dual_operand_result(status),
        .y = ti_c54x_dual_operand_result(status)
    };
    return result;
}

static bool
ti_c54x_dual_mode_is_valid(ti_c54x_indirect_dual_mode mode)
{
    return mode >= TI_C54X_INDIRECT_DUAL_MOD0 &&
           mode <= TI_C54X_INDIRECT_DUAL_MOD3;
}

static bool
ti_c54x_dual_selector_is_valid(uint8_t ar_index)
{
    return ar_index >= UINT8_C(2) && ar_index <= UINT8_C(5);
}

static bool
ti_c54x_dual_access_is_valid(ti_c54x_address_access access)
{
    return access >= TI_C54X_ADDRESS_ACCESS_READ &&
           access <= TI_C54X_ADDRESS_ACCESS_WRITE;
}

static bool
ti_c54x_dual_update_crosses_boundary(ti_c54x_indirect_dual_mode mode,
                                     uint16_t ar_value)
{
    return (mode == TI_C54X_INDIRECT_DUAL_MOD1 && ar_value == UINT16_C(0)) ||
           (mode == TI_C54X_INDIRECT_DUAL_MOD2 &&
            ar_value == UINT16_MAX);
}

static ti_c54x_address_update
ti_c54x_dual_ar_update(ti_c54x_indirect_dual_mode mode,
                       uint8_t ar_index,
                       uint16_t ar_value)
{
    const uint16_t value_after =
        mode == TI_C54X_INDIRECT_DUAL_MOD1
            ? (uint16_t)(ar_value - UINT16_C(1))
            : (uint16_t)(ar_value + UINT16_C(1));
    const ti_c54x_address_update update = {
        .target = TI_C54X_ADDRESS_UPDATE_AR,
        .phase = TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
        .ar_index = ar_index,
        .value_before = ar_value,
        .value_after = value_after};
    return update;
}

static ti_c54x_address_result
ti_c54x_dual_word16_operand(uint8_t ar_index,
                            uint16_t ar_value,
                            ti_c54x_indirect_dual_mode mode)
{
    ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {.operand = {0}}
    };

    result.effect.operand.width = TI_C54X_DATA_OPERAND_WIDTH_WORD16;
    result.effect.operand.first_word = ar_value;
    result.effect.operand.second_word = UINT16_C(0);
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_AR;
    result.effect.source.value.ar.index = ar_index;
    result.effect.source.value.ar.value_before = ar_value;
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_NONE;
    if (mode != TI_C54X_INDIRECT_DUAL_MOD0) {
        result.effect.update = ti_c54x_dual_ar_update(mode, ar_index, ar_value);
    }
    return result;
}

ti_c54x_indirect_dual_result
ti_c54x_indirect_dual_word16(
    uint8_t x_ar_index,
    uint16_t x_ar_value,
    uint8_t y_ar_index,
    uint16_t y_ar_value,
    ti_c54x_indirect_dual_mode x_mode,
    ti_c54x_indirect_dual_mode y_mode,
    ti_c54x_address_access access)
{
    ti_c54x_indirect_dual_result result;

    /* Intrinsic discriminants are checked before unsupported values so a
     * malformed call cannot be mistaken for a recognized architectural mode. */
    if (!ti_c54x_dual_mode_is_valid(x_mode) ||
        !ti_c54x_dual_mode_is_valid(y_mode) ||
        !ti_c54x_dual_selector_is_valid(x_ar_index) ||
        !ti_c54x_dual_selector_is_valid(y_ar_index) ||
        !ti_c54x_dual_access_is_valid(access)) {
        return ti_c54x_dual_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    if (x_mode == TI_C54X_INDIRECT_DUAL_MOD3 ||
        y_mode == TI_C54X_INDIRECT_DUAL_MOD3) {
        return ti_c54x_dual_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
    }
    if (access != TI_C54X_ADDRESS_ACCESS_READ) {
        return ti_c54x_dual_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
    }
    if (x_ar_index == y_ar_index && x_ar_value != y_ar_value) {
        return ti_c54x_dual_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }

    /* A selector alias has one architectural register write. X mode is the
     * owner of that write; Y still reads the same frozen old address but has
     * no independent boundary or update. Distinct selectors plan separately. */
    if (ti_c54x_dual_update_crosses_boundary(x_mode, x_ar_value) ||
        (x_ar_index != y_ar_index &&
         ti_c54x_dual_update_crosses_boundary(y_mode, y_ar_value))) {
        return ti_c54x_dual_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    }

    result.status = TI_C54X_ADDRESS_STATUS_OK;
    result.x = ti_c54x_dual_word16_operand(x_ar_index, x_ar_value, x_mode);
    result.y = ti_c54x_dual_word16_operand(
        y_ar_index, y_ar_value,
        x_ar_index == y_ar_index ? TI_C54X_INDIRECT_DUAL_MOD0 : y_mode);
    return result;
}
