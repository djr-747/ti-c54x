/*
 * Module contract
 *
 * Owns: executable proof of the pure repeat-only request gate across inactive,
 * active, boundary, request-class, invalid, and transactional cases.
 * Does not own: general interrupt admission, IFR state, acknowledgement,
 * priority, vectors, devices, RC, PC, timing, or test discovery.
 */

#include "../../../../src/sequencer/repeat/repeat_cardinality.h"
#include "../../../../src/sequencer/repeat/repeat_request_gate.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static void
check_gate(uint32_t body_executions_remaining,
           ti_c54x_sequencer_repeat_request_kind request_kind,
           ti_c54x_sequencer_repeat_request_decision expected)
{
    ti_c54x_sequencer_repeat_request_decision decision =
        TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;

    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_request_gate_evaluate(
            body_executions_remaining, request_kind, &decision) ==
        TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_OK);
    TI_C54X_TEST_CHECK(decision == expected);
}

static void
test_inactive_gate_blocks_nothing(void)
{
    check_gate(
        UINT32_C(0), TI_C54X_SEQUENCER_REPEAT_REQUEST_MASKABLE_INTERRUPT,
        TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);
    check_gate(
        UINT32_C(0), TI_C54X_SEQUENCER_REPEAT_REQUEST_NMI,
        TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);
    check_gate(
        UINT32_C(0), TI_C54X_SEQUENCER_REPEAT_REQUEST_LITERAL_RS_RESET,
        TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);
}

static void
test_active_gate_defers_interrupts_but_not_reset(void)
{
    const uint32_t remaining_values[] = {
        UINT32_C(1), UINT32_C(257),
        TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT};
    uint32_t index;

    for (index = UINT32_C(0);
         index < (uint32_t)(sizeof(remaining_values) /
                            sizeof(remaining_values[0]));
         ++index) {
        check_gate(
            remaining_values[index],
            TI_C54X_SEQUENCER_REPEAT_REQUEST_MASKABLE_INTERRUPT,
            TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER);
        check_gate(
            remaining_values[index], TI_C54X_SEQUENCER_REPEAT_REQUEST_NMI,
            TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER);
        check_gate(
            remaining_values[index],
            TI_C54X_SEQUENCER_REPEAT_REQUEST_LITERAL_RS_RESET,
            TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);
    }
}

static void
test_invalid_inputs_preserve_output(void)
{
    ti_c54x_sequencer_repeat_request_decision decision;

    decision = TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER;
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_request_gate_evaluate(
            TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT + UINT32_C(1),
            TI_C54X_SEQUENCER_REPEAT_REQUEST_NMI, &decision) ==
        TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_BODY_EXECUTION_COUNT);
    TI_C54X_TEST_CHECK(
        decision == TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER);

    decision = TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED;
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_request_gate_evaluate(
            UINT32_C(1), TI_C54X_SEQUENCER_REPEAT_REQUEST_INVALID,
            &decision) ==
        TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_REQUEST_KIND);
    TI_C54X_TEST_CHECK(
        decision == TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);

    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_request_gate_evaluate(
            UINT32_C(1), TI_C54X_SEQUENCER_REPEAT_REQUEST_KIND_COUNT,
            &decision) ==
        TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_REQUEST_KIND);
    TI_C54X_TEST_CHECK(
        decision == TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);

    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_request_gate_evaluate(
            UINT32_C(1), (ti_c54x_sequencer_repeat_request_kind)-1,
            &decision) ==
        TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_REQUEST_KIND);
    TI_C54X_TEST_CHECK(
        decision == TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);

    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_request_gate_evaluate(
            UINT32_C(1), TI_C54X_SEQUENCER_REPEAT_REQUEST_NMI, NULL) ==
        TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_INVALID_ARGUMENT);
}

int
main(void)
{
    test_inactive_gate_blocks_nothing();
    test_active_gate_defers_interrupts_but_not_reset();
    test_invalid_inputs_preserve_output();
    return 0;
}
