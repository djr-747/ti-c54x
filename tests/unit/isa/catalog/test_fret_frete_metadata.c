/* Module contract
 *
 * Owns: executable proof that the FRET/FRETE far-return catalog
 * rows carry the 0xF4E4/0xF6E4/0xF4E5/0xF6E5 values under the
 * 0xFFFF mask with one program word and the NONE layout,
 * STACK_POINTER inputs, and ineligible repeatability, covering
 * the full first-word population per row with
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
    const ti_c54x_opcode_metadata *fret =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FRET);
    const ti_c54x_opcode_metadata *fretd =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FRETD);
    const ti_c54x_opcode_metadata *frete =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FRETE);
    const ti_c54x_opcode_metadata *freted =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FRETED);

    TI_C54X_TEST_CHECK(fret != NULL);
    TI_C54X_TEST_CHECK(fret->id == TI_C54X_OPCODE_ID_FRET);
    TI_C54X_TEST_CHECK(fret->first_word_value == UINT16_C(0xF4E4));
    TI_C54X_TEST_CHECK(fret->first_word_mask == UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(fret->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(fret->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(fret->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(fret->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK(fretd != NULL);
    TI_C54X_TEST_CHECK(fretd->id == TI_C54X_OPCODE_ID_FRETD);
    TI_C54X_TEST_CHECK(fretd->first_word_value == UINT16_C(0xF6E4));
    TI_C54X_TEST_CHECK(fretd->first_word_mask == UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(fretd->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(fretd->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(fretd->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(fretd->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK(frete != NULL);
    TI_C54X_TEST_CHECK(frete->id == TI_C54X_OPCODE_ID_FRETE);
    TI_C54X_TEST_CHECK(frete->first_word_value == UINT16_C(0xF4E5));
    TI_C54X_TEST_CHECK(frete->first_word_mask == UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(frete->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(frete->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(frete->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(frete->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK(freted != NULL);
    TI_C54X_TEST_CHECK(freted->id == TI_C54X_OPCODE_ID_FRETED);
    TI_C54X_TEST_CHECK(freted->first_word_value == UINT16_C(0xF6E5));
    TI_C54X_TEST_CHECK(freted->first_word_mask == UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(freted->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(freted->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(freted->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(freted->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
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
                           UINT16_C(0xF4E4), UINT16_C(0xFFFF),
                           TI_C54X_OPCODE_ID_FRET) == 1U);
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xF6E4), UINT16_C(0xFFFF),
                           TI_C54X_OPCODE_ID_FRETD) == 1U);
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xF4E5), UINT16_C(0xFFFF),
                           TI_C54X_OPCODE_ID_FRETE) == 1U);
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xF6E5), UINT16_C(0xFFFF),
                           TI_C54X_OPCODE_ID_FRETED) == 1U);
}

static void
test_row_boundaries(void)
{
    /* The four far-return rows sit beside the FBACC/FBACCD
     * accumulator rows (0xF4E6/0xF6E6 under 0xFEFF) and the
     * near RETE/RETED rows (0xF4EB/0xF6EB): every neighbor
     * decodes to its own row. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4E4)) == TI_C54X_OPCODE_ID_FRET);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4E5)) == TI_C54X_OPCODE_ID_FRETE);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4E6)) == TI_C54X_OPCODE_ID_FBACC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4EB)) == TI_C54X_OPCODE_ID_RETE);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF6E4)) == TI_C54X_OPCODE_ID_FRETD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF6E5)) == TI_C54X_OPCODE_ID_FRETED);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF6E6)) == TI_C54X_OPCODE_ID_FBACCD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF6EB)) == TI_C54X_OPCODE_ID_RETED);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
