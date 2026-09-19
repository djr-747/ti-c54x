/*
 * Module contract
 *
 * Owns: checked CLKMD cell storage with reset-unknown
 * knownness and the PLLSTATUS-bit write mask.
 * Does not own: address mapping, request routing, opcode
 * semantics, CPU lifecycle, timing, allocation, observers,
 * or logging.
 */

#include "clock_mode_register.h"

#include <stddef.h>
#include <stdint.h>

ti_c54x_clkmd_status
ti_c54x_clkmd_reset(ti_c54x_clkmd_state *cell)
{
    if (cell == NULL) {
        return TI_C54X_CLKMD_INVALID_ARGUMENT;
    }
    cell->value = UINT16_C(0);
    cell->known = UINT8_C(0);
    return TI_C54X_CLKMD_OK;
}

ti_c54x_clkmd_status
ti_c54x_clkmd_read(const ti_c54x_clkmd_state *cell,
                   uint16_t *value)
{
    if (cell == NULL || value == NULL) {
        return TI_C54X_CLKMD_INVALID_ARGUMENT;
    }
    if (cell->known == UINT8_C(0)) {
        return TI_C54X_CLKMD_UNKNOWN;
    }
    *value = cell->value;
    return TI_C54X_CLKMD_OK;
}

ti_c54x_clkmd_status
ti_c54x_clkmd_write(ti_c54x_clkmd_state *cell,
                    uint16_t value)
{
    if (cell == NULL) {
        return TI_C54X_CLKMD_INVALID_ARGUMENT;
    }
    cell->value =
        (uint16_t)(value & TI_C54X_CLKMD_WRITABLE_MASK);
    cell->known = UINT8_C(1);
    return TI_C54X_CLKMD_OK;
}
