/*
 * Module contract
 *
 * Owns: executable proof that the pure repeat-cardinality policy widens every
 * uint16_t operand before deriving its one-based uint32_t body count.
 * Does not own: body execution, RC or PC state, opcode effects, timing,
 * interrupts, buses, memory, diagnostics, or test discovery.
 */

#include "../../../../src/sequencer/repeat/repeat_cardinality.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

_Static_assert(TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT ==
                   UINT32_C(65536),
               "maximum repeat cardinality must preserve operand plus one");

static void
test_complete_operand_domain(void)
{
    uint32_t operand;

    for (operand = UINT32_C(0); operand <= (uint32_t)UINT16_MAX; ++operand) {
        ti_c54x_sequencer_repeat_cardinality cardinality = {UINT32_C(0)};

        TI_C54X_TEST_CHECK(
            ti_c54x_sequencer_repeat_cardinality_from_operand(
                (uint16_t)operand, &cardinality) ==
            TI_C54X_SEQUENCER_REPEAT_CARDINALITY_OK);
        TI_C54X_TEST_CHECK_U64(
            cardinality.body_execution_count, operand + UINT32_C(1));
    }
}

static void
test_endpoints_and_null_output(void)
{
    ti_c54x_sequencer_repeat_cardinality cardinality = {UINT32_C(0)};

    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_cardinality_from_operand(
            UINT16_C(0), &cardinality) ==
        TI_C54X_SEQUENCER_REPEAT_CARDINALITY_OK);
    TI_C54X_TEST_CHECK_U64(cardinality.body_execution_count, UINT32_C(1));

    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_cardinality_from_operand(
            UINT16_MAX, &cardinality) ==
        TI_C54X_SEQUENCER_REPEAT_CARDINALITY_OK);
    TI_C54X_TEST_CHECK_U64(
        cardinality.body_execution_count,
        TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT);

    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_cardinality_from_operand(
            UINT16_C(0), NULL) ==
        TI_C54X_SEQUENCER_REPEAT_CARDINALITY_INVALID_ARGUMENT);
}

int
main(void)
{
    test_complete_operand_domain();
    test_endpoints_and_null_output();
    return 0;
}
