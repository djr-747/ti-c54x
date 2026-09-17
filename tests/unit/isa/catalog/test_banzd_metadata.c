/*
 * Module contract
 *
 * Owns: executable proof that the BANZD catalog row carries the
 * 0x6E00 value under the 0xFF00 mask with two program words, the
 * shared Sind-plus-pmad layout, auxiliary-register inputs, and
 * ineligible repeatability, covering all 256 Sind encodings with
 * exact population and disjointness against every other row.
 * Does not own: catalog generation, decode, Sind evaluation, AR
 * writes, CPU/state mutation, fetch or bus behavior, sequencing,
 * timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t BANZD_VALUE = UINT16_C(0x6E00);
static const uint16_t BANZD_MASK = UINT16_C(0xFF00);
/* Bit 9 clear selects the non-delayed BANZ sibling row. */
static const uint16_t BANZ_VALUE = UINT16_C(0x6C00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_BANZD);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 201);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_BANZD);
    TI_C54X_TEST_CHECK(metadata->first_word_value == BANZD_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == BANZD_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_SIND_PMAD16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
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
            (uint16_t)(word & BANZD_MASK) == BANZD_VALUE;
        ti_c54x_opcode_id id;

        if (matches_row) {
            ++matches;
        }
        id = ti_c54x_opcode_catalog_lookup_first_word(word);
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_BANZD) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 256U);
}

static void
test_delayed_variant_separation(void)
{
    /* Clearing bit 9 reaches the BANZ sibling, never this row. */
    ti_c54x_opcode_id id =
        ti_c54x_opcode_catalog_lookup_first_word(BANZ_VALUE);

    TI_C54X_TEST_CHECK(id == TI_C54X_OPCODE_ID_BANZ);
    id = ti_c54x_opcode_catalog_lookup_first_word(
        (uint16_t)(BANZD_VALUE | UINT16_C(0x008C)));
    TI_C54X_TEST_CHECK(id == TI_C54X_OPCODE_ID_BANZD);
}

int
main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    test_delayed_variant_separation();
    return 0;
}
