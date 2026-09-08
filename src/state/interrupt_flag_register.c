/*
 * Module contract
 *
 * Owns: checked IFR storage access and exact-one-bit set/clear mutation.
 * Does not own: interrupt sources, vectors, priority, repeat arbitration, CPU
 * lifecycle, timing, buses, allocation, observers, or logging.
 */

#include "interrupt_flag_register.h"

#include <stddef.h>
#include <stdint.h>

static int
is_one_bit(uint16_t mask)
{
    return mask != UINT16_C(0) && (mask & (uint16_t)(mask - UINT16_C(1))) ==
                                      UINT16_C(0);
}

ti_c54x_arch_state_status
ti_c54x_arch_state_read_ifr(const ti_c54x_arch_state *state,
                            uint16_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->scalar_known_mask & TI_C54X_ARCH_STATE_KNOWN_IFR) == 0U) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = state->ifr;
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_ifr(ti_c54x_arch_state *state,
                             uint16_t value)
{
    if (state == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->ifr = value;
    state->scalar_known_mask = (uint8_t)(state->scalar_known_mask |
                                         TI_C54X_ARCH_STATE_KNOWN_IFR);
    return TI_C54X_ARCH_STATE_OK;
}

static ti_c54x_arch_state_status
modify_ifr_bit(ti_c54x_arch_state *state, uint16_t mask, int set)
{
    uint16_t value;

    if (state == NULL || !is_one_bit(mask)) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if (ti_c54x_arch_state_read_ifr(state, &value) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    value = set ? (uint16_t)(value | mask) : (uint16_t)(value & (uint16_t)~mask);
    return ti_c54x_arch_state_write_ifr(state, value);
}

ti_c54x_arch_state_status
ti_c54x_arch_state_set_ifr_bit(ti_c54x_arch_state *state,
                               uint16_t mask)
{
    return modify_ifr_bit(state, mask, 1);
}

ti_c54x_arch_state_status
ti_c54x_arch_state_clear_ifr_bit(ti_c54x_arch_state *state,
                                 uint16_t mask)
{
    return modify_ifr_bit(state, mask, 0);
}
