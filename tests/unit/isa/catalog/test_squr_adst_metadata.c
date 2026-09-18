/*
 * Module contract
 *
 * Owns: executable proof that the SQUR_ADST catalog row carries the
 * 0xF48D value under the 0xFEFF mask with one program word, the
 * reused MAX/MIN single-destination-bit layout, accumulator/FRCT/
 * OVM inputs, and eligible repeatability, covering both first-word
 * encodings with no shadow word.
 * Does not own: catalog generation, decode, accumulator/status
 * state, CPU sequencing, fetch or bus behavior, timing, or handler
 * bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t SQUR_ADST_VALUE = UINT16_C(0xF48D);
static const uint16_t SQUR_ADST_MASK = UINT16_C(0xFEFF);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_SQUR_ADST);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 278);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_SQUR_ADST);
    TI_C54X_TEST_CHECK(metadata->first_word_value == SQUR_ADST_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == SQUR_ADST_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_MAX_MIN_ACC);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_FRCT_OVM);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x0100));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & SQUR_ADST_MASK) == SQUR_ADST_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_SQUR_ADST) == matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 2U);
}

static void
test_row_boundaries(void)
{
    /* The 0xF48C neighborhood belongs to MPYA_DST, not SQUR_ADST. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF48C)) == TI_C54X_OPCODE_ID_MPYA_DST);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF58C)) == TI_C54X_OPCODE_ID_MPYA_DST);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF48D)) == TI_C54X_OPCODE_ID_SQUR_ADST);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF58D)) == TI_C54X_OPCODE_ID_SQUR_ADST);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
