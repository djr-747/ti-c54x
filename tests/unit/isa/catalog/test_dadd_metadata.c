/*
 * Module contract
 *
 * Owns: executable proof that the DADD catalog row carries the
 * 0x5000 value under the 0xFC00 mask with one program word, the
 * shared source/destination/indirect/addr7 layout, accumulator
 * plus SXM/OVM/C16 inputs, and unresolved repeatability,
 * covering exactly the 1024 Lmem first-word encodings with the
 * 0x4FFF UNKNOWN lower sibling and the 0x5400 DSUB upper
 * sibling pinned.
 * Does not own: catalog generation, decode, memory addressing or
 * buses, accumulator/status state, CPU sequencing, timing, or
 * handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t DADD_VALUE = UINT16_C(0x5000);
static const uint16_t DADD_MASK = UINT16_C(0xFC00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_DADD);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 375);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_DADD);
    TI_C54X_TEST_CHECK(metadata->first_word_value == DADD_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == DADD_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM_C16);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x03FF));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & DADD_MASK) == DADD_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_DADD) == matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 1024U);
}

static void
test_row_boundaries(void)
{
    /* 0x5000 and 0x53FF are the Lmem slice ends; the lower
     * neighbor decodes UNKNOWN while the upper neighbor now
     * decodes the P3-S100 DSUB row. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x5000)) == TI_C54X_OPCODE_ID_DADD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x53FF)) == TI_C54X_OPCODE_ID_DADD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x4FFF)) == TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x5400)) == TI_C54X_OPCODE_ID_DSUB);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
