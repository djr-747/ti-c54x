/*
 * Module contract
 *
 * Owns: executable proof that the MAC_SMEM_LK catalog row carries
 * the 0x6400 value under the 0xFC00 mask with two program words,
 * the 2-word source/destination/cookie/lk layout, accumulator plus
 * FRCT/OVM inputs without T, and eligible repeatability, covering
 * all 1,024 first-word encodings with no shadow word.
 * Does not own: catalog generation, decode, accumulator/T/status
 * state, CPU sequencing, fetch or bus behavior, timing, or handler
 * bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t MAC_SMEM_LK_VALUE = UINT16_C(0x6400);
static const uint16_t MAC_SMEM_LK_MASK = UINT16_C(0xFC00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MAC_SMEM_LK);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 280);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_MAC_SMEM_LK);
    TI_C54X_TEST_CHECK(metadata->first_word_value == MAC_SMEM_LK_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == MAC_SMEM_LK_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SOURCE_DESTINATION_INDIRECT_ADDR7_EXTENSION16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_FRCT_OVM);
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
            (uint16_t)(word & MAC_SMEM_LK_MASK) == MAC_SMEM_LK_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_MAC_SMEM_LK) == matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 1024U);
}

static void
test_row_boundaries(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x6400)) ==
                       TI_C54X_OPCODE_ID_MAC_SMEM_LK);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x67FF)) ==
                       TI_C54X_OPCODE_ID_MAC_SMEM_LK);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x63FF)) !=
                       TI_C54X_OPCODE_ID_MAC_SMEM_LK);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x6800)) !=
                       TI_C54X_OPCODE_ID_MAC_SMEM_LK);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
