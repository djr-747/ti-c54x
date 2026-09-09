/*
 * Module contract
 *
 * Owns: executable COMMON_TI proof that an active single-repeat window
 * defers maskable requests and NMI while still admitting literal RS/reset,
 * and that no request is blocked once every body execution completes.
 * Evidence: E-TI-C54X-SPRU131G-S6-7-P6-20-6-22-RPT-IRQ and
 * E-TI-C54X-SPRU131G-S6-10-7-P6-34-RPT-IRQ-COMPLETION.
 * Does not own: request sampling, IFR state, acknowledgement, priority,
 * vectors, device wiring, predecode arbitration, RC, PC, timing, buses,
 * memory, allocation, observers, or diagnostics. The caller owns the
 * output; NOT_BLOCKED is not service.
 */

#include "sequencer/repeat/repeat_cardinality.h"
#include "sequencer/repeat/repeat_request_gate.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
check_decision(uint32_t remaining,
               ti_c54x_sequencer_repeat_request_kind kind,
               ti_c54x_sequencer_repeat_request_decision expected)
{
    ti_c54x_sequencer_repeat_request_decision decision =
        TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_request_gate_evaluate(
                           remaining, kind, &decision) ==
                       TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_OK);
    TI_C54X_TEST_CHECK(decision == expected);
}

/* While any body execution remains, maskable requests and NMI defer but
 * literal RS/reset is still admitted. Proved at both count endpoints. */
static void
test_active_window_defers_interrupts_but_not_reset(void)
{
    static const uint32_t endpoints[] = {UINT32_C(1),
                                         TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT};
    size_t index;

    for (index = 0U; index < sizeof(endpoints) / sizeof(endpoints[0]);
         ++index) {
        check_decision(
            endpoints[index],
            TI_C54X_SEQUENCER_REPEAT_REQUEST_MASKABLE_INTERRUPT,
            TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER);
        check_decision(endpoints[index],
                       TI_C54X_SEQUENCER_REPEAT_REQUEST_NMI,
                       TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER);
        check_decision(endpoints[index],
                       TI_C54X_SEQUENCER_REPEAT_REQUEST_LITERAL_RS_RESET,
                       TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);
    }
}

/* Once every body execution completes, no request is blocked. */
static void
test_completed_window_blocks_nothing(void)
{
    check_decision(UINT32_C(0),
                   TI_C54X_SEQUENCER_REPEAT_REQUEST_MASKABLE_INTERRUPT,
                   TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);
    check_decision(UINT32_C(0), TI_C54X_SEQUENCER_REPEAT_REQUEST_NMI,
                   TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);
    check_decision(UINT32_C(0),
                   TI_C54X_SEQUENCER_REPEAT_REQUEST_LITERAL_RS_RESET,
                   TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);
}

static void
test_invalid_inputs_rejected_transactionally(void)
{
    ti_c54x_sequencer_repeat_request_decision decision =
        TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;
    unsigned char before[sizeof(decision)];

    memcpy(before, &decision, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_request_gate_evaluate(
                           UINT32_C(1),
                           TI_C54X_SEQUENCER_REPEAT_REQUEST_MASKABLE_INTERRUPT,
                           NULL) ==
                       TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_request_gate_evaluate(
                           TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT +
                               UINT32_C(1),
                           TI_C54X_SEQUENCER_REPEAT_REQUEST_NMI,
                           &decision) ==
                       TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_BODY_EXECUTION_COUNT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_request_gate_evaluate(
                           UINT32_C(1),
                           TI_C54X_SEQUENCER_REPEAT_REQUEST_INVALID,
                           &decision) ==
                       TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_REQUEST_KIND);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_request_gate_evaluate(
                           UINT32_C(1),
                           TI_C54X_SEQUENCER_REPEAT_REQUEST_KIND_COUNT,
                           &decision) ==
                       TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_REQUEST_KIND);
    TI_C54X_TEST_CHECK(memcmp(before, &decision, sizeof(decision)) == 0);
}

int
main(void)
{
    test_active_window_defers_interrupts_but_not_reset();
    test_completed_window_blocks_nothing();
    test_invalid_inputs_rejected_transactionally();
    puts("repeat_request_gate_common: ok");
    return 0;
}
