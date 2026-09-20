/*
 * Module contract
 *
 * Owns: executable proof of the NMI take-arbitration truth table
 * (P3-S94): a pending NMI takes only when no repeat is ACTIVE
 * and no delayed-branch countdown is armed; INTM never matters.
 * Does not own: the take itself, maskable arbitration, priority,
 * vectors, CPU flow, timing, buses, allocation, or diagnostics.
 */

#include "sequencer/interrupt/take_arbitration.h"

#include "../../../support/test_harness.h"

#include <stdbool.h>

static void
test_idle_fetches(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_take_arbitrate_nmi(false, false, false) ==
                       TI_C54X_TAKE_ARBITRATION_FETCH);
    TI_C54X_TEST_CHECK(ti_c54x_take_arbitrate_nmi(false, true, false) ==
                       TI_C54X_TAKE_ARBITRATION_FETCH);
    TI_C54X_TEST_CHECK(ti_c54x_take_arbitrate_nmi(false, false, true) ==
                       TI_C54X_TAKE_ARBITRATION_FETCH);
    TI_C54X_TEST_CHECK(ti_c54x_take_arbitrate_nmi(false, true, true) ==
                       TI_C54X_TAKE_ARBITRATION_FETCH);
}

static void
test_quiet_pending_takes(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_take_arbitrate_nmi(true, false, false) ==
                       TI_C54X_TAKE_ARBITRATION_TAKE_NMI);
}

static void
test_busy_pending_defers(void)
{
    /* TI SPRU131G section 6.7: NMI is disabled until the repeat
     * loop completes; multicycle delay slots likewise delay it. */
    TI_C54X_TEST_CHECK(ti_c54x_take_arbitrate_nmi(true, true, false) ==
                       TI_C54X_TAKE_ARBITRATION_DEFER);
    TI_C54X_TEST_CHECK(ti_c54x_take_arbitrate_nmi(true, false, true) ==
                       TI_C54X_TAKE_ARBITRATION_DEFER);
    TI_C54X_TEST_CHECK(ti_c54x_take_arbitrate_nmi(true, true, true) ==
                       TI_C54X_TAKE_ARBITRATION_DEFER);
}

int
main(void)
{
    test_idle_fetches();
    test_quiet_pending_takes();
    test_busy_pending_defers();
    return 0;
}
