/* Module contract
 *
 * Owns: executable proof that the FCALL/FCALA far-call catalog
 * rows carry the 0xF980/0xFB80 values under the 0xFF80 mask with
 * two program words and the BRANCH_FAR_PMAD layout plus the
 * 0xF4E7/0xF6E7 values under the 0xFEFF mask with one program
 * word and the ACCUMULATOR_SOURCE layout, STACK_POINTER (plus
 * selected-accumulator) inputs, and ineligible repeatability,
 * covering the full first-word population per row with
 * UNKNOWN/sibling neighbors.
 * Does not own: catalog generation, decode, memory addressing or
 * buses, status state, CPU sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *fcall_pmad =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FCALL_PMAD);
    const ti_c54x_opcode_metadata *fcalld =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FCALLD);
    const ti_c54x_opcode_metadata *fcala =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FCALA);
    const ti_c54x_opcode_metadata *fcalad =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FCALAD);

    TI_C54X_TEST_CHECK(fcall_pmad != NULL);
    TI_C54X_TEST_CHECK(fcall_pmad->id == TI_C54X_OPCODE_ID_FCALL_PMAD);
    TI_C54X_TEST_CHECK(fcall_pmad->first_word_value == UINT16_C(0xF980));
    TI_C54X_TEST_CHECK(fcall_pmad->first_word_mask == UINT16_C(0xFF80));
    TI_C54X_TEST_CHECK(fcall_pmad->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(fcall_pmad->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_FAR_PMAD);
    TI_C54X_TEST_CHECK(fcall_pmad->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(fcall_pmad->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK((uint16_t)~fcall_pmad->first_word_mask ==
                       UINT16_C(0x007F));
    TI_C54X_TEST_CHECK(fcalld != NULL);
    TI_C54X_TEST_CHECK(fcalld->id == TI_C54X_OPCODE_ID_FCALLD);
    TI_C54X_TEST_CHECK(fcalld->first_word_value == UINT16_C(0xFB80));
    TI_C54X_TEST_CHECK(fcalld->first_word_mask == UINT16_C(0xFF80));
    TI_C54X_TEST_CHECK(fcalld->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(fcalld->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_FAR_PMAD);
    TI_C54X_TEST_CHECK(fcalld->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(fcalld->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK(fcala != NULL);
    TI_C54X_TEST_CHECK(fcala->id == TI_C54X_OPCODE_ID_FCALA);
    TI_C54X_TEST_CHECK(fcala->first_word_value == UINT16_C(0xF4E7));
    TI_C54X_TEST_CHECK(fcala->first_word_mask == UINT16_C(0xFEFF));
    TI_C54X_TEST_CHECK(fcala->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(fcala->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(fcala->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(fcala->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STACK_POINTER);
    TI_C54X_TEST_CHECK(fcalad != NULL);
    TI_C54X_TEST_CHECK(fcalad->id == TI_C54X_OPCODE_ID_FCALAD);
    TI_C54X_TEST_CHECK(fcalad->first_word_value == UINT16_C(0xF6E7));
    TI_C54X_TEST_CHECK(fcalad->first_word_mask == UINT16_C(0xFEFF));
    TI_C54X_TEST_CHECK(fcalad->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(fcalad->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(fcalad->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(fcalad->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STACK_POINTER);
}

static unsigned
check_population(uint16_t value, uint16_t mask, ti_c54x_opcode_id expected_id)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row = (uint16_t)(word & mask) == value;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            TI_C54X_TEST_CHECK(id == expected_id);
            matches++;
        } else if (id == expected_id) {
            TI_C54X_TEST_CHECK(false);
        }
    }
    return matches;
}

static void
test_exact_population(void)
{
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xF980), UINT16_C(0xFF80),
                           TI_C54X_OPCODE_ID_FCALL_PMAD) == 128U);
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xFB80), UINT16_C(0xFF80),
                           TI_C54X_OPCODE_ID_FCALLD) == 128U);
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xF4E7), UINT16_C(0xFEFF),
                           TI_C54X_OPCODE_ID_FCALA) == 2U);
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xF6E7), UINT16_C(0xFEFF),
                           TI_C54X_OPCODE_ID_FCALAD) == 2U);
}

static void
test_row_boundaries(void)
{
    /* 0xF4E7/0xF5E7 are FCALA A/B; 0xF6E7/0xF7E7 are FCALAD A/B;
     * 0xF980 tiles FCALL through 0xF9FF and 0xFB80 tiles FCALLD
     * through 0xFBFF, disjoint from the tightened CC/CCD rows. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4E7)) == TI_C54X_OPCODE_ID_FCALA);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF5E7)) == TI_C54X_OPCODE_ID_FCALA);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF6E7)) == TI_C54X_OPCODE_ID_FCALAD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF7E7)) == TI_C54X_OPCODE_ID_FCALAD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF980)) == TI_C54X_OPCODE_ID_FCALL_PMAD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF9FF)) == TI_C54X_OPCODE_ID_FCALL_PMAD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xFB80)) == TI_C54X_OPCODE_ID_FCALLD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xFBFF)) == TI_C54X_OPCODE_ID_FCALLD);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
