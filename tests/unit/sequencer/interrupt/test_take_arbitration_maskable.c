/*
 * Module contract
 *
 * Owns: executable proof of the maskable take-arbitration truth
 * table (P3-S95): exactly one known-enabled routed source takes
 * naming its bit; INTM set/unknown, IMR/IFR unknown, nothing
 * enabled, or a missing vector never take; multiple enabled
 * sources or a busy repeat/delay defer with edges preserved.
 * Does not own: the take itself, NMI arbitration, vectors,
 * priority policy, CPU flow, timing, buses, allocation, or
 * diagnostics.
 */

#include "sequencer/interrupt/take_arbitration.h"

#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static ti_c54x_take_maskable_snapshot
quiet_snapshot(void)
{
    ti_c54x_take_maskable_snapshot snapshot = {0};

    snapshot.intm_known = true;
    snapshot.intm_value = UINT8_C(0);
    snapshot.ifr_known = true;
    snapshot.ifr_value = UINT16_C(0x0200);
    snapshot.imr_known = true;
    snapshot.imr_value = UINT16_C(0x0200);
    snapshot.vectors_known_mask = UINT16_C(0x0200);
    return snapshot;
}

static void
test_null_fetches(void)
{
    ti_c54x_take_maskable_result result =
        ti_c54x_take_arbitrate_maskable(NULL);

    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_FETCH);
}

static void
test_quiet_single_takes_naming_bit(void)
{
    ti_c54x_take_maskable_snapshot snapshot = quiet_snapshot();
    ti_c54x_take_maskable_result result =
        ti_c54x_take_arbitrate_maskable(&snapshot);

    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_TAKE);
    TI_C54X_TEST_CHECK(result.source_bit == UINT8_C(9));
}

static void
test_masked_or_unknown_fetches(void)
{
    ti_c54x_take_maskable_snapshot snapshot = quiet_snapshot();
    ti_c54x_take_maskable_result result;

    /* INTM set masks everything. */
    snapshot.intm_value = UINT8_C(1);
    result = ti_c54x_take_arbitrate_maskable(&snapshot);
    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_FETCH);
    snapshot.intm_value = UINT8_C(0);

    /* INTM unknown cannot prove unmasked. */
    snapshot.intm_known = false;
    result = ti_c54x_take_arbitrate_maskable(&snapshot);
    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_FETCH);
    snapshot.intm_known = true;

    /* IMR unknown cannot prove enabled. */
    snapshot.imr_known = false;
    result = ti_c54x_take_arbitrate_maskable(&snapshot);
    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_FETCH);
    snapshot.imr_known = true;

    /* IFR unknown cannot prove pending. */
    snapshot.ifr_known = false;
    result = ti_c54x_take_arbitrate_maskable(&snapshot);
    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_FETCH);
    snapshot.ifr_known = true;

    /* Masked-out bit is not enabled. */
    snapshot.imr_value = UINT16_C(0x0001);
    result = ti_c54x_take_arbitrate_maskable(&snapshot);
    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_FETCH);
    snapshot.imr_value = UINT16_C(0x0200);

    /* Nothing pending at all. */
    snapshot.ifr_value = UINT16_C(0x0000);
    result = ti_c54x_take_arbitrate_maskable(&snapshot);
    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_FETCH);
}

static void
test_routing_gap_defers(void)
{
    ti_c54x_take_maskable_snapshot snapshot = quiet_snapshot();
    ti_c54x_take_maskable_result result;

    /* Pending and enabled but with no routed entry address. */
    snapshot.vectors_known_mask = UINT16_C(0x0000);
    result = ti_c54x_take_arbitrate_maskable(&snapshot);
    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_DEFER);
}

static void
test_priority_gap_defers(void)
{
    /* Two enabled sources: priority is device-owned, so the
     * core takes neither and preserves both edges. */
    ti_c54x_take_maskable_snapshot snapshot = quiet_snapshot();
    ti_c54x_take_maskable_result result;

    snapshot.ifr_value = UINT16_C(0x0201);
    snapshot.imr_value = UINT16_C(0x0201);
    snapshot.vectors_known_mask = UINT16_C(0x0201);
    result = ti_c54x_take_arbitrate_maskable(&snapshot);
    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_DEFER);
}

static void
test_busy_defers(void)
{
    ti_c54x_take_maskable_snapshot snapshot = quiet_snapshot();
    ti_c54x_take_maskable_result result;

    snapshot.repeat_active = true;
    result = ti_c54x_take_arbitrate_maskable(&snapshot);
    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_DEFER);
    snapshot.repeat_active = false;

    snapshot.delayed_branch_armed = true;
    result = ti_c54x_take_arbitrate_maskable(&snapshot);
    TI_C54X_TEST_CHECK(result.decision ==
                       TI_C54X_TAKE_MASKABLE_DEFER);
}

int
main(void)
{
    test_null_fetches();
    test_quiet_single_takes_naming_bit();
    test_masked_or_unknown_fetches();
    test_routing_gap_defers();
    test_priority_gap_defers();
    test_busy_defers();
    return 0;
}
