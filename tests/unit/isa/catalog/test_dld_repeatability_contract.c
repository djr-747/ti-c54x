/*
 * Module contract
 *
 * Owns: executable proof of the evidence-bounded DLD/DST repeatability
 * asymmetry, admission mapping, and unchanged generated trusted bindings.
 * Does not own: opcode decoding, semantic arithmetic, CPU state, sequencing,
 * timing, interrupt policy, or generated lookup production.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/ops/dispatch/trusted_opcode_binding.h"
#include "../../../../src/sequencer/repeat/repeat_body_eligibility.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static void
check_row(ti_c54x_opcode_id id,
          uint16_t value,
          ti_c54x_repeatability_class repeatability,
          ti_c54x_sequencer_repeat_body_admission admission)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(id);
    const ti_c54x_trusted_opcode_binding *binding =
        ti_c54x_trusted_opcode_binding_for_id(id);
    ti_c54x_sequencer_repeat_body_admission actual_admission =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == id);
    TI_C54X_TEST_CHECK(metadata->first_word_value == value);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFEF8));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->repeatability == repeatability);
    TI_C54X_TEST_CHECK(binding != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_trusted_opcode_binding_is_canonical(binding));
    TI_C54X_TEST_CHECK(binding->handler != NULL);
    TI_C54X_TEST_CHECK(binding->metadata.id == id);
    TI_C54X_TEST_CHECK(binding->metadata.repeatability == repeatability);
    TI_C54X_TEST_CHECK(binding->metadata.first_word_value == value);
    TI_C54X_TEST_CHECK(binding->metadata.first_word_mask ==
                       UINT16_C(0xFEF8));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_repeat_body_admission_from_class(
            repeatability, &actual_admission) ==
        TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_OK);
    TI_C54X_TEST_CHECK(actual_admission == admission);
}

static void
test_dld_is_unresolved_and_dst_remains_ineligible(void)
{
    check_row(TI_C54X_OPCODE_ID_DLD_LONG32_MOD13, UINT16_C(0x56E8),
              TI_C54X_REPEATABILITY_UNRESOLVED,
              TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_BLOCK_UNRESOLVED);
    check_row(TI_C54X_OPCODE_ID_DST_LONG32_MOD13, UINT16_C(0x4EE8),
              TI_C54X_REPEATABILITY_INELIGIBLE,
              TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
}

int
main(void)
{
    test_dld_is_unresolved_and_dst_remains_ineligible();
    puts("dld_repeatability_contract: ok");
    return 0;
}
