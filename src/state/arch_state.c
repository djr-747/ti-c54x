/* Module contract
 * Owns: deterministic construction of the private common reset state,
 * including its wide program-word PC carrier, and checked knownness-aware
 * AR/SP/RTN plus delegated ARP/DP/CPL/CMPT status-field access and
 * mutation. BK storage is part of the aggregate but its accessors live
 * in block_size_register.c. RTN resets to zero-but-unknown under the
 * shared SPRU131G §6.9 policy: no reset value is adopted for it.
 * Does not own: CPU lifecycle or configured bootstrap reset, raw ST0/ST1/PMST
 * status-register APIs, address planning or sequencer policy, effect sequencing,
 * bus transfers, opcode semantics, diagnostics, or public ABI declarations.
 */

#include "arch_state.h"

#include <stddef.h>
#include <string.h>

static uint8_t ar_known_bit(uint8_t index)
{
    return (uint8_t)(UINT8_C(1) << index);
}

ti_c54x_arch_state_status
ti_c54x_arch_state_reset_common(ti_c54x_arch_state *state)
{
    if (state == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }

    memset(state, 0, sizeof(*state));
    state->pc_word = UINT16_C(0xFF80);
    state->ifr = UINT16_C(0);
    state->rc = UINT16_C(0);
    if (ti_c54x_status_register_reset_common(&state->status) !=
        TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    /* SWWSR adopts its documented SPRU131 Table 10-2 reset value. */
    if (ti_c54x_swwsr_reset(&state->swwsr) != TI_C54X_SWWSR_OK) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->scalar_known_mask = (uint8_t)(TI_C54X_ARCH_STATE_KNOWN_PC |
                                         TI_C54X_ARCH_STATE_KNOWN_IFR |
                                         TI_C54X_ARCH_STATE_KNOWN_RC);
    state->ar_known_mask = UINT8_C(0);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_ar(const ti_c54x_arch_state *state,
                           uint8_t index,
                           uint16_t *value)
{
    if (state == NULL || value == NULL || index >= 8U) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->ar_known_mask & ar_known_bit(index)) == 0U) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = state->ar[index];
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_ar(ti_c54x_arch_state *state,
                            uint8_t index,
                            uint16_t value)
{
    if (state == NULL || index >= 8U) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->ar[index] = value;
    state->ar_known_mask = (uint8_t)(state->ar_known_mask | ar_known_bit(index));
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_sp(const ti_c54x_arch_state *state,
                           uint16_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->scalar_known_mask & TI_C54X_ARCH_STATE_KNOWN_SP) == 0U) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = state->sp;
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_sp(ti_c54x_arch_state *state,
                            uint16_t value)
{
    if (state == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->sp = value;
    state->scalar_known_mask =
        (uint8_t)(state->scalar_known_mask | TI_C54X_ARCH_STATE_KNOWN_SP);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_rtn(const ti_c54x_arch_state *state,
                            uint16_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->scalar_known_mask & TI_C54X_ARCH_STATE_KNOWN_RTN) == 0U) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = state->rtn;
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_rtn(ti_c54x_arch_state *state,
                             uint16_t value)
{
    if (state == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->rtn = value;
    state->scalar_known_mask =
        (uint8_t)(state->scalar_known_mask | TI_C54X_ARCH_STATE_KNOWN_RTN);
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_arp(const ti_c54x_arch_state *state,
                            uint8_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    const ti_c54x_status_register_field_result result =
        ti_c54x_status_register_read_arp(&state->status);
    if (result.status != TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = (uint8_t)result.raw_value;
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_arp(ti_c54x_arch_state *state,
                             uint8_t value)
{
    if (state == NULL || value > UINT8_C(7)) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    return ti_c54x_status_register_write_arp(&state->status, value) ==
                   TI_C54X_STATUS_REGISTER_OK
               ? TI_C54X_ARCH_STATE_OK
               : TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_dp(const ti_c54x_arch_state *state,
                           uint16_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    const ti_c54x_status_register_field_result result =
        ti_c54x_status_register_read_dp(&state->status);
    if (result.status != TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = result.raw_value;
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_dp(ti_c54x_arch_state *state,
                            uint16_t value)
{
    if (state == NULL || value > TI_C54X_ST0_DP_MASK) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    return ti_c54x_status_register_write_dp(&state->status, value) ==
                   TI_C54X_STATUS_REGISTER_OK
               ? TI_C54X_ARCH_STATE_OK
               : TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_cpl(const ti_c54x_arch_state *state,
                            uint8_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    const ti_c54x_status_register_field_result result =
        ti_c54x_status_register_read_cpl(&state->status);
    if (result.status != TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = (uint8_t)result.raw_value;
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_cpl(ti_c54x_arch_state *state,
                             uint8_t value)
{
    if (state == NULL || value > UINT8_C(1)) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    return ti_c54x_status_register_write_cpl(&state->status, value) ==
                   TI_C54X_STATUS_REGISTER_OK
               ? TI_C54X_ARCH_STATE_OK
               : TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_cmpt(const ti_c54x_arch_state *state,
                             uint8_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    const ti_c54x_status_register_field_result result =
        ti_c54x_status_register_read_cmpt(&state->status);
    if (result.status != TI_C54X_STATUS_REGISTER_OK) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = (uint8_t)result.raw_value;
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_cmpt(ti_c54x_arch_state *state,
                              uint8_t value)
{
    if (state == NULL || value > UINT8_C(1)) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    return ti_c54x_status_register_write_cmpt(&state->status, value) ==
                   TI_C54X_STATUS_REGISTER_OK
               ? TI_C54X_ARCH_STATE_OK
               : TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
}
