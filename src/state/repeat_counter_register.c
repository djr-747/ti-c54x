/*
 * Module contract
 *
 * Owns: defined-width checked access to architectural RC and its dedicated
 * knownness bit. Does not own: repeat sequencing, effect validation, CPU
 * lifecycle, program flow, timing, buses, allocation, observers, or logging.
 */

#include "repeat_counter_register.h"

#include <stddef.h>
#include <stdint.h>

ti_c54x_arch_state_status
ti_c54x_arch_state_read_rc(const ti_c54x_arch_state *state,
                           uint16_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->scalar_known_mask & TI_C54X_ARCH_STATE_KNOWN_RC) == 0U) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = state->rc;
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_rc(ti_c54x_arch_state *state,
                            uint16_t value)
{
    if (state == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->rc = value;
    state->scalar_known_mask = (uint8_t)(state->scalar_known_mask |
                                         TI_C54X_ARCH_STATE_KNOWN_RC);
    return TI_C54X_ARCH_STATE_OK;
}
