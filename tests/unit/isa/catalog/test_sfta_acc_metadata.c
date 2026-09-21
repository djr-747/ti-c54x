/*
 * Module contract
 *
 * Owns: executable proof that the SFTA_ACC catalog row carries
 * the 0xF460 value under the 0xFCE0 mask with one program word,
 * the shared accumulator source/destination/shift5 layout,
 * accumulator plus SXM plus OVM input resources, and unresolved
 * repeatability, covering all 128 first-word encodings with no
 * shadow word.
 * Does not own: catalog generation, decode, accumulator/status
 * state, CPU sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t SFTA_ACC_VALUE = UINT16_C(0xF460);
static const uint16_t SFTA_ACC_MASK = UINT16_C(0xFCE0);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_SFTA_ACC);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 351);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_SFTA_ACC);
    TI_C54X_TEST_CHECK(metadata->first_word_value == SFTA_ACC_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == SFTA_ACC_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x031F));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & SFTA_ACC_MASK) == SFTA_ACC_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_SFTA_ACC) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 128U);
}

static void
test_row_boundaries(void)
{
    /* 0xF45F belongs to LD_SRC_SHIFT; 0xF480 to ADD_SRC_ASM. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF45F)) ==
                       TI_C54X_OPCODE_ID_LD_SRC_SHIFT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF480)) ==
                       TI_C54X_OPCODE_ID_ADD_SRC_ASM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF460)) ==
                       TI_C54X_OPCODE_ID_SFTA_ACC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF47F)) ==
                       TI_C54X_OPCODE_ID_SFTA_ACC);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
