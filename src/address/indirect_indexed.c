/*
 * Module contract
 *
 * Owns: checked CMPT=0 single-operand WORD16 effective-address planning for
 * AR0-indexed MOD5 and MOD6 forms, including their ordered one-register
 * post-access effects.
 * Does not own: CPU state or ARP/CMPT validation, register storage or
 * mutation, bus transfers, opcode decoding, wrapping/circular arithmetic,
 * long32 operands, instruction scheduling, or diagnostics.
 */

#include "indirect_indexed.h"

#include <stdbool.h>
#include <stdint.h>

static ti_c54x_address_result
ti_c54x_indexed_result_with_status(ti_c54x_address_status status)
{
    const ti_c54x_address_result result = {
        .status = status,
        .effect = {.operand = {0}}
    };
    return result;
}

static bool
ti_c54x_indexed_mode_valid(ti_c54x_indirect_indexed_mode mode)
{
    return mode == TI_C54X_INDIRECT_INDEXED_MOD5 ||
           mode == TI_C54X_INDIRECT_INDEXED_MOD6;
}

static bool
ti_c54x_indexed_access_valid(ti_c54x_address_access access)
{
    return access == TI_C54X_ADDRESS_ACCESS_READ ||
           access == TI_C54X_ADDRESS_ACCESS_WRITE;
}

static ti_c54x_address_update
ti_c54x_indexed_ar_update(uint8_t ar_index,
                          uint16_t value_before,
                          uint16_t value_after)
{
    const ti_c54x_address_update update = {
        .target = TI_C54X_ADDRESS_UPDATE_AR,
        .phase = TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
        .ar_index = ar_index,
        .value_before = value_before,
        .value_after = value_after
    };
    return update;
}

ti_c54x_address_result
ti_c54x_indirect_indexed_word(uint8_t ar_index,
                              uint16_t ar_value,
                              uint16_t ar0_value,
                              ti_c54x_indirect_indexed_mode mode,
                              ti_c54x_address_access access)
{
    ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {.operand = {0}}
    };
    uint16_t value_after;

    /* Validate discriminants before the selected-AR alias case. */
    if (ar_index > UINT8_C(7) || !ti_c54x_indexed_mode_valid(mode) ||
        !ti_c54x_indexed_access_valid(access)) {
        return ti_c54x_indexed_result_with_status(
            TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
    }
    /* AR0 self-indexing ordering is deliberately deferred. */
    if (ar_index == UINT8_C(0)) {
        return ti_c54x_indexed_result_with_status(
            TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS);
    }

    if (mode == TI_C54X_INDIRECT_INDEXED_MOD5) {
        if (ar_value < ar0_value) {
            return ti_c54x_indexed_result_with_status(
                TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
        }
        value_after = (uint16_t)(ar_value - ar0_value);
    } else {
        const uint32_t sum = (uint32_t)ar_value + (uint32_t)ar0_value;
        if (sum > UINT32_C(0xffff)) {
            return ti_c54x_indexed_result_with_status(
                TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
        }
        value_after = (uint16_t)sum;
    }

    result.effect.operand = ti_c54x_data_operand_word16(ar_value);
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_AR_INDEXED;
    result.effect.source.value.ar_indexed.index = ar_index;
    result.effect.source.value.ar_indexed.value_before = ar_value;
    result.effect.source.value.ar_indexed.ar0_value_before = ar0_value;
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0;
    result.effect.update = ti_c54x_indexed_ar_update(
        ar_index, ar_value, value_after);
    return result;
}
