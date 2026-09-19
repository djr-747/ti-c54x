/*
 * Module contract
 *
 * Owns: executable proof that the RPT_K catalog row carries the
 * 0xEC00 value under the 0xFF00 mask with one program word, the
 * 1-word 8-bit-K layout, no inputs, and ineligible
 * repeatability, covering all 256 first-word encodings with no
 * shadow word.
 * Does not own: catalog generation, decode, RC state, CPU
 * sequencing, fetch or bus behavior, timing, or handler
 * bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t RPT_K_VALUE = UINT16_C(0xEC00);
static const uint16_t RPT_K_MASK = UINT16_C(0xFF00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RPT_K);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 309);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_RPT_K);
    TI_C54X_TEST_CHECK(metadata->first_word_value == RPT_K_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == RPT_K_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x00FF));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    unsigned rpt_words = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & RPT_K_MASK) == RPT_K_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_RPT_K) == matches_row);
        if (matches_row) {
            ++rpt_words;
        }
    }
    TI_C54X_TEST_CHECK(matches == 256U);
    TI_C54X_TEST_CHECK(rpt_words == 256U);
}

static void
test_row_boundaries(void)
{
    /* 0xEBFF ends below; 0xED00 starts above. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xEBFF)) != TI_C54X_OPCODE_ID_RPT_K);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xED00)) != TI_C54X_OPCODE_ID_RPT_K);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xEC00)) == TI_C54X_OPCODE_ID_RPT_K);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xECFF)) == TI_C54X_OPCODE_ID_RPT_K);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
