/*
 * Module contract
 *
 * Owns: executable proof that the CCD catalog row carries the
 * 0xFB00 value under the 0xFF00 mask with two program words, the
 * shared condition-plus-pmad layout, combined accumulator and
 * status plus stack-pointer inputs, and ineligible
 * repeatability, covering all 256 condition encodings with exact
 * population and disjointness against every other row.
 * Does not own: catalog generation, decode, condition
 * evaluation, stack writes, CPU/state mutation, fetch or bus
 * behavior, sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t CCD_VALUE = UINT16_C(0xFB00);
static const uint16_t CCD_MASK = UINT16_C(0xFF00);
/* Bit 9 clear selects the non-delayed CC sibling. */
static const uint16_t CC_VALUE = UINT16_C(0xF900);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_CCD);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 159);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_CCD);
    TI_C54X_TEST_CHECK(metadata->first_word_value == CCD_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == CCD_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_COND_PMAD16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC_STACK_POINTER);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x00FF));
}

static void
test_exact_population_and_disjointness(void)
{
    unsigned matches = 0U;

    for (uint32_t raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & CCD_MASK) == CCD_VALUE;
        ti_c54x_opcode_id id;

        if (matches_row) {
            ++matches;
        }
        id = ti_c54x_opcode_catalog_lookup_first_word(word);
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_CCD) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 256U);
}

static void
test_non_delayed_variant_separation(void)
{
    /* Clearing bit 9 reaches the non-delayed CC sibling, never
     * this row. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           CC_VALUE) == TI_C54X_OPCODE_ID_CC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(CCD_VALUE | UINT16_C(0x0030))) ==
                       TI_C54X_OPCODE_ID_CCD);
}

int
main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    test_non_delayed_variant_separation();
    return 0;
}
