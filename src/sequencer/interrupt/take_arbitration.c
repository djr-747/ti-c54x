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
