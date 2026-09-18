/*
 * Module contract
 *
 * Owns: executable proof that the RPT_LK catalog row carries the
 * exact 0xF070 first word with two program words, the 2-word
 * 16-bit-lk layout, no inputs, and ineligible repeatability,
 * with no shadow word.
 * Does not own: catalog generation, decode, RC state, CPU
 * sequencing, fetch or bus behavior, timing, or handler
 * bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t RPT_LK_VALUE = UINT16_C(0xF070);
static const uint16_t RPT_LK_MASK = UINT16_C(0xFFFF);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RPT_LK);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 273);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_RPT_LK);
    TI_C54X_TEST_CHECK(metadata->first_word_value == RPT_LK_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == RPT_LK_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x0000));
}

static void
test_exact_population(void)
{
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row = word == RPT_LK_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_RPT_LK) == matches_row);
    }
}

static void
test_row_boundaries(void)
{
    /* Near branches stay outside: 0xF073 is B_PMAD, 0xF074 is CALL_PMAD. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF06F)) != TI_C54X_OPCODE_ID_RPT_LK);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF071)) != TI_C54X_OPCODE_ID_RPT_LK);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF073)) == TI_C54X_OPCODE_ID_B_PMAD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF070)) == TI_C54X_OPCODE_ID_RPT_LK);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
