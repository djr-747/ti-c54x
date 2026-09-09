/*
 * Module contract
 *
 * Owns: the shared dual-indirect MOD3 post-access AR computation used by
 * the address planner and the root commit validator.
 * Does not own: operand planning, selector/mode validation, register
 * storage, sequencing, timing, diagnostics, or any other addressing mode.
 */

#include "indirect_dual_mod3.h"

#include <stddef.h>
#include <stdint.h>

/* Return the least power-of-two EFB size strictly greater than BK. BK is
 * 16-bit, so the largest result is 2^16 and uint32_t is sufficient. */
static uint32_t
ti_c54x_dual_mod3_efb_size(uint16_t bk_value)
{
    uint32_t efb_size = UINT32_C(1);

    while (efb_size <= (uint32_t)bk_value) {
        efb_size <<= 1U;
    }
    return efb_size;
}

ti_c54x_address_status
ti_c54x_indirect_dual_mod3_after(uint16_t ar_value,
                                uint16_t ar0_value,
                                uint16_t bk_value,
                                uint16_t *value_after)
{
    /* AR0 is a signed step: the quantity added to or subtracted from ARx. */
    const int32_t step = (int32_t)(int16_t)ar0_value;

    if (value_after == NULL) {
        return TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT;
    }
    if (bk_value == UINT16_C(0)) {
        const int32_t moved = (int32_t)ar_value + step;

        /* Plain indexed addition performs no wrap; out-of-range results
         * stay an explicit boundary, mirroring single-indexed policy. */
        if (moved < INT32_C(0) || moved > INT32_C(0xffff)) {
            return TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY;
        }
        *value_after = (uint16_t)moved;
        return TI_C54X_ADDRESS_STATUS_OK;
    }
    {
        const uint32_t efb_size = ti_c54x_dual_mod3_efb_size(bk_value);
        const uint32_t efb_mask = efb_size - UINT32_C(1);
        const uint32_t ar32 = (uint32_t)ar_value;
        const uint32_t index = ar32 & efb_mask;
        int32_t moved = (int32_t)index + step;
        uint32_t result;

        /* First access must point inside the buffer. */
        if (index >= (uint32_t)bk_value) {
            return TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY;
        }
        /* The documented step rule admits magnitudes up to BK; larger steps
         * leave the single-fold algorithm undefined and stay a boundary. */
        if (step > (int32_t)bk_value || step < -(int32_t)bk_value) {
            return TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY;
        }
        if (moved < INT32_C(0)) {
            moved += (int32_t)bk_value;
        } else if ((uint32_t)moved >= (uint32_t)bk_value) {
            moved -= (int32_t)bk_value;
        }
        result = (ar32 & ~efb_mask) + (uint32_t)moved;
        if (result > UINT32_C(0xffff)) {
            return TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY;
        }
        *value_after = (uint16_t)result;
        return TI_C54X_ADDRESS_STATUS_OK;
    }
}
