/*
 * Module contract
 *
 * Owns: executable proof that the pure repeat-body policy consumes only the
 * catalog's explicit repeatability scalar and fails transactionally.
 * Does not own: concrete opcode classifications, decode, body execution, RC,
 * PC, interrupts, buses, memory, diagnostics, or test discovery.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/sequencer/repeat/repeat_body_eligibility.h"
#include "../../../support/test_harness.h"

static void
check_decision(ti_c54x_repeatability_class repeatability,
               ti_c54x_sequencer_repeat_body_admission expected)
{
    ti_c54x_sequencer_repeat_body_admission decision =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;

    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_body_admission_from_class(
            repeatability, &decision) ==
        TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_OK);
    TI_C54X_TEST_CHECK(decision == expected);
}

static void
test_all_declared_classes(void)
{
    check_decision(
        TI_C54X_REPEATABILITY_UNRESOLVED,
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_BLOCK_UNRESOLVED);
    check_decision(
        TI_C54X_REPEATABILITY_ELIGIBLE,
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT);
    check_decision(
        TI_C54X_REPEATABILITY_INELIGIBLE,
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
}

static void
test_invalid_inputs_preserve_output(void)
{
    ti_c54x_sequencer_repeat_body_admission decision;

    decision = TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_body_admission_from_class(
            TI_C54X_REPEATABILITY_COUNT, &decision) ==
        TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_INVALID_REPEATABILITY);
    TI_C54X_TEST_CHECK(
        decision == TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT);

    decision = TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE;
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_body_admission_from_class(
            (ti_c54x_repeatability_class)-1, &decision) ==
        TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_INVALID_REPEATABILITY);
    TI_C54X_TEST_CHECK(
        decision ==
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);

    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_body_admission_from_class(
            TI_C54X_REPEATABILITY_ELIGIBLE, NULL) ==
        TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_INVALID_ARGUMENT);
}

int
main(void)
{
    test_all_declared_classes();
    test_invalid_inputs_preserve_output();
    return 0;
}
