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
#include "indirect_dual_mod3.h"

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
                            ti_c54x_indirect_dual_mode mode,
                            uint16_t ar0_value,
                            uint16_t bk_value)
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
    if (mode == TI_C54X_INDIRECT_DUAL_MOD0) {
        return result;
    }
    if (mode == TI_C54X_INDIRECT_DUAL_MOD3) {
        uint16_t value_after = UINT16_C(0);
        const ti_c54x_address_status status =
            ti_c54x_indirect_dual_mod3_after(ar_value, ar0_value, bk_value,
                                            &value_after);

        if (status != TI_C54X_ADDRESS_STATUS_OK) {
            return ti_c54x_dual_operand_result(status);
        }
        /* The documented BK=0/AR0=0 case performs no modification. */
        if (ar0_value == UINT16_C(0) && bk_value == UINT16_C(0)) {
            return result;
        }
        result.effect.update = (ti_c54x_address_update){
            .target = TI_C54X_ADDRESS_UPDATE_AR,
            .phase = TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
            .ar_index = ar_index,
            .value_before = ar_value,
            .value_after = value_after};
        return result;
    }
    result.effect.update = ti_c54x_dual_ar_update(mode, ar_index, ar_value);
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
    uint16_t ar0_value,
    uint16_t bk_value,
    ti_c54x_address_access access)
{
    ti_c54x_indirect_dual_result result;
    ti_c54x_address_result x_side;
    ti_c54x_address_result y_side;
    const ti_c54x_indirect_dual_mode y_planned_mode =
        x_ar_index == y_ar_index ? TI_C54X_INDIRECT_DUAL_MOD0 : y_mode;

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
    /* Address arithmetic is direction-independent: reads and writes plan
     * the same operand address and AR update. The access discriminant
     * stays validated above; callers keep their own READ/WRITE gates. */
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

    x_side = ti_c54x_dual_word16_operand(x_ar_index, x_ar_value, x_mode,
                                        ar0_value, bk_value);
    if (x_side.status != TI_C54X_ADDRESS_STATUS_OK) {
        return ti_c54x_dual_result_with_status(x_side.status);
    }
    y_side = ti_c54x_dual_word16_operand(y_ar_index, y_ar_value,
                                        y_planned_mode, ar0_value, bk_value);
    if (y_side.status != TI_C54X_ADDRESS_STATUS_OK) {
        return ti_c54x_dual_result_with_status(y_side.status);
    }

    result.status = TI_C54X_ADDRESS_STATUS_OK;
    result.x = x_side;
    result.y = y_side;
    return result;
}
