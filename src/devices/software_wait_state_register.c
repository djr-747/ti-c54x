/*
 * Module contract
 *
 * Owns: checked SWWSR cell storage with documented-reset
 * knownness and the reserved-bit-15 write mask.
 * Does not own: address mapping, request routing, opcode
 * semantics, CPU lifecycle, timing, allocation, observers,
 * or logging.
 */

#include "software_wait_state_register.h"

#include <stddef.h>
#include <stdint.h>

ti_c54x_swwsr_status
ti_c54x_swwsr_reset(ti_c54x_swwsr_state *cell)
{
    if (cell == NULL) {
        return TI_C54X_SWWSR_INVALID_ARGUMENT;
    }
    cell->value = TI_C54X_SWWSR_RESET_VALUE;
    cell->known = UINT8_C(1);
    return TI_C54X_SWWSR_OK;
}

ti_c54x_swwsr_status
ti_c54x_swwsr_read(const ti_c54x_swwsr_state *cell,
                   uint16_t *value)
{
    if (cell == NULL || value == NULL) {
        return TI_C54X_SWWSR_INVALID_ARGUMENT;
    }
    if (cell->known == UINT8_C(0)) {
        return TI_C54X_SWWSR_UNKNOWN;
    }
    *value = cell->value;
    return TI_C54X_SWWSR_OK;
}

ti_c54x_swwsr_status
ti_c54x_swwsr_write(ti_c54x_swwsr_state *cell,
                    uint16_t value)
{
    if (cell == NULL) {
        return TI_C54X_SWWSR_INVALID_ARGUMENT;
    }
    cell->value =
        (uint16_t)(value & TI_C54X_SWWSR_WRITABLE_MASK);
    cell->known = UINT8_C(1);
    return TI_C54X_SWWSR_OK;
}
