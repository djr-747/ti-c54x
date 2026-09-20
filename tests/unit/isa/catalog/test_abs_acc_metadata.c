/*
 * Module contract
 *
 * Owns: executable proof that the ABS_ACC catalog row carries
 * the 0xF485 value under the 0xFCFF mask with one program word,
 * the shared accumulator source/destination layout, accumulator
 * plus OVM input resources, and unresolved repeatability,
 * covering all 4 first-word encodings with no shadow word.
 * Does not own: catalog generation, decode, accumulator/status
 * state, CPU sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t ABS_ACC_VALUE = UINT16_C(0xF485);
static const uint16_t ABS_ACC_MASK = UINT16_C(0xFCFF);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ABS_ACC);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 344);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_ABS_ACC);
    TI_C54X_TEST_CHECK(metadata->first_word_value == ABS_ACC_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == ABS_ACC_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_OVM);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x0300));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & ABS_ACC_MASK) == ABS_ACC_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_ABS_ACC) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 4U);
}

static void
test_row_boundaries(void)
{
    /* 0xF484 belongs to NEG_ACC; 0xF483 to SAT_ACC. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF484)) ==
                       TI_C54X_OPCODE_ID_NEG_ACC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF483)) ==
                       TI_C54X_OPCODE_ID_SAT_ACC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF485)) ==
                       TI_C54X_OPCODE_ID_ABS_ACC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF785)) ==
                       TI_C54X_OPCODE_ID_ABS_ACC);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
