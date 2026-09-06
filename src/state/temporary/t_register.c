/*
 * Module contract
 *
 * Owns: checked mutation and knownness-aware access for the private T
 * temporary-register state. Reset clears physical storage and clears
 * architectural knownness; unknown reads do not expose stale storage.
 * Does not own: MMR or data-memory routing, CPU lifecycle, opcode semantics,
 * effect sequencing, timing, diagnostics, persistence, or public ABI state.
 */

#include "t_register.h"

#include <stddef.h>

ti_c54x_t_register_status
ti_c54x_t_register_reset(ti_c54x_t_register_state *state)
{
    if (state == NULL) {
        return TI_C54X_T_REGISTER_INVALID_ARGUMENT;
    }

    state->raw_value = UINT16_C(0);
    state->known = false;
    return TI_C54X_T_REGISTER_OK;
}

ti_c54x_t_register_status
ti_c54x_t_register_read(const ti_c54x_t_register_state *state,
                        uint16_t *value)
{
    if (state == NULL || value == NULL) {
        return TI_C54X_T_REGISTER_INVALID_ARGUMENT;
    }
    if (!state->known) {
        return TI_C54X_T_REGISTER_UNKNOWN;
    }

    *value = state->raw_value;
    return TI_C54X_T_REGISTER_OK;
}

ti_c54x_t_register_status
ti_c54x_t_register_write(ti_c54x_t_register_state *state, uint16_t value)
{
    if (state == NULL) {
        return TI_C54X_T_REGISTER_INVALID_ARGUMENT;
    }

    state->raw_value = value;
    state->known = true;
    return TI_C54X_T_REGISTER_OK;
}
