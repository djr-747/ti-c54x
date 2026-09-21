/*
 * Module contract
 *
 * Owns: checked access and dedicated knownness mutation for the private 16-bit
 * XPC program-counter extension register.
 * Does not own: architectural-state construction or reset, MMR routing, far
 * control semantics, CPU lifecycle, buses, diagnostics, or public
 * ABI declarations.
 */

#include "program_counter_extension.h"

#include <stddef.h>
#include <stdint.h>

ti_c54x_arch_state_status
ti_c54x_arch_state_read_xpc(const ti_c54x_arch_state *state,
                            uint16_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    if ((state->scalar_known_mask & TI_C54X_ARCH_STATE_KNOWN_XPC) == 0U) {
        return TI_C54X_ARCH_STATE_UNKNOWN;
    }
    *value = state->xpc;
    return TI_C54X_ARCH_STATE_OK;
}

ti_c54x_arch_state_status
ti_c54x_arch_state_write_xpc(ti_c54x_arch_state *state,
                             uint16_t value)
{
    if (state == NULL) {
        return TI_C54X_ARCH_STATE_INVALID_ARGUMENT;
    }
    state->xpc = value;
    state->scalar_known_mask = (uint8_t)(state->scalar_known_mask |
                                         TI_C54X_ARCH_STATE_KNOWN_XPC);
    return TI_C54X_ARCH_STATE_OK;
}
