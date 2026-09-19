/*
 * Module contract
 *
 * Owns: executable proof that the MACA_T catalog row carries the
 * 0xF488 value under the 0xFCFE mask with one program word each,
 * the shared source/destination/round layout, accumulator/T/FRCT/OVM
 * inputs, and eligible repeatability, covering all 8 first-word
 * encodings with no shadow word.
 * Does not own: catalog generation, decode, accumulator/T/status
 * state, CPU sequencing, fetch or bus behavior, timing, or handler
 * bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t MACA_T_VALUE = UINT16_C(0xF488);
static const uint16_t MACA_T_MASK = UINT16_C(0xFCFE);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MACA_T);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 334);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_MACA_T);
    TI_C54X_TEST_CHECK(metadata->first_word_value == MACA_T_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == MACA_T_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_ROUND);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x0301));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & MACA_T_MASK) == MACA_T_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_MACA_T) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 8U);
}

static void
test_row_boundaries(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF480)) != TI_C54X_OPCODE_ID_MACA_T);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF48A)) != TI_C54X_OPCODE_ID_MACA_T);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF488)) == TI_C54X_OPCODE_ID_MACA_T);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF789)) == TI_C54X_OPCODE_ID_MACA_T);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
