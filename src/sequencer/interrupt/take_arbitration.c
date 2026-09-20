/*
 * Module contract
 *
 * Owns: the pure predecode take-arbitration decision described in
 * take_arbitration.h.
 * Does not own: pending state, repeat/delay state, vectors, the
 * take itself, CPU flow, timing, buses, or diagnostics.
 */

#include "take_arbitration.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

ti_c54x_take_arbitration_decision ti_c54x_take_arbitrate_nmi(
    bool nmi_pending, bool repeat_active, bool delayed_branch_armed)
{
    if (!nmi_pending) {
        return TI_C54X_TAKE_ARBITRATION_FETCH;
    }
    if (repeat_active || delayed_branch_armed) {
        return TI_C54X_TAKE_ARBITRATION_DEFER;
    }
    return TI_C54X_TAKE_ARBITRATION_TAKE_NMI;
}

/* Count the set bits of the enabled set, reporting the single
 * index when exactly one is set. */
static unsigned
enabled_sources(uint16_t enabled, uint8_t *single_bit)
{
    unsigned count = 0U;
    uint8_t bit = UINT8_C(0);

    for (bit = UINT8_C(0); bit < UINT8_C(16); bit++) {
        if ((enabled & (uint16_t)((uint16_t)1U << bit)) != UINT16_C(0)) {
            count++;
            *single_bit = bit;
        }
    }
    return count;
}

ti_c54x_take_maskable_result ti_c54x_take_arbitrate_maskable(
    const ti_c54x_take_maskable_snapshot *snapshot)
{
    ti_c54x_take_maskable_result result = {
        .decision = TI_C54X_TAKE_MASKABLE_FETCH, .source_bit = UINT8_C(0)};
    uint16_t enabled = UINT16_C(0);
    uint8_t single_bit = UINT8_C(0);
    unsigned count = 0U;

    if (snapshot == NULL) {
        return result;
    }
    if (snapshot->repeat_active || snapshot->delayed_branch_armed) {
        result.decision = TI_C54X_TAKE_MASKABLE_DEFER;
        return result;
    }
    /* Masked or unprovable: no take wants to fire. */
    if (!snapshot->intm_known || snapshot->intm_value != UINT8_C(0) ||
        !snapshot->imr_known || !snapshot->ifr_known) {
        return result;
    }
    enabled =
        (uint16_t)(snapshot->ifr_value & snapshot->imr_value);
    count = enabled_sources(enabled, &single_bit);
    if (count == 0U) {
        return result;
    }
    if (count > 1U) {
        /* Priority is device-owned: take neither, keep both. */
        result.decision = TI_C54X_TAKE_MASKABLE_DEFER;
        return result;
    }
    if ((snapshot->vectors_known_mask &
         (uint16_t)((uint16_t)1U << single_bit)) == UINT16_C(0)) {
        result.decision = TI_C54X_TAKE_MASKABLE_DEFER;
        return result;
    }
    result.decision = TI_C54X_TAKE_MASKABLE_TAKE;
    result.source_bit = single_bit;
    return result;
}
