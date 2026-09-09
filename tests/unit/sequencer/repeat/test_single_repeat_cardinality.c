/*
 * Module contract
 *
 * Owns: executable COMMON_TI proof of the evidenced single-repeat
 * cardinality examples and widen-before-add derivation.
 * Evidence: E-TI-C54X-SPRU172C-S4-RPT-P4-146-4-147 (`#2` executes three
 * bodies, `#0xffff` executes 65,536 bodies).
 * Does not own: body execution, RC/PC state, interrupts, timing, buses,
 * memory, allocation, observers, or diagnostics. The caller owns the
 * output.
 */

#include "sequencer/repeat/repeat_cardinality.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static void
check_cardinality(uint16_t operand, uint32_t expected_count)
{
    ti_c54x_sequencer_repeat_cardinality cardinality = {UINT32_C(0)};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_cardinality_from_operand(
                           operand, &cardinality) ==
                       TI_C54X_SEQUENCER_REPEAT_CARDINALITY_OK);
    TI_C54X_TEST_CHECK_U64(cardinality.body_execution_count, expected_count);
}

static void
test_evidenced_examples(void)
{
    /* Operand zero still executes the body once. */
    check_cardinality(UINT16_C(0), UINT32_C(1));
    /* The documented `#2` example executes three bodies. */
    check_cardinality(UINT16_C(2), UINT32_C(3));
    /* The documented `#0xffff` example executes 65,536 bodies. */
    check_cardinality(UINT16_C(0xFFFF), UINT32_C(65536));
}

static void
test_widen_before_add_has_no_wrap(void)
{
    /* 0xFFFF + 1 must widen to 65,536, never wrap to zero. */
    check_cardinality(UINT16_C(0xFFFE), UINT32_C(0xFFFF));
    check_cardinality(UINT16_C(0xFFFF), UINT32_C(65536));
    TI_C54X_TEST_CHECK(UINT32_C(65536) ==
                       TI_C54X_SEQUENCER_REPEAT_MAX_BODY_EXECUTION_COUNT);
}

static void
test_null_output_rejected(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_repeat_cardinality_from_operand(
                           UINT16_C(0), NULL) ==
                       TI_C54X_SEQUENCER_REPEAT_CARDINALITY_INVALID_ARGUMENT);
}

int
main(void)
{
    test_evidenced_examples();
    test_widen_before_add_has_no_wrap();
    test_null_output_rejected();
    puts("repeat_cardinality_common: ok");
    return 0;
}
