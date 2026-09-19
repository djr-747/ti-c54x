/*
 * Module contract
 *
 * Owns: executable proof that the ST_LK_SMEM catalog row carries
 * the 0x7600 value under the 0xFF80 mask with two program words,
 * the shared Smem,#lk layout, no input resources, and eligible
 * repeatability, covering exactly the 128 direct first-word
 * encodings with no shadow word and no overlap into the
 * carved-out ST_IMMEDIATE_SMEM subrange.
 * Does not own: catalog generation, decode, memory addressing or
 * buses, accumulator/T/status state, CPU sequencing, timing, or
 * handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t ST_LK_SMEM_VALUE = UINT16_C(0x7600);
static const uint16_t ST_LK_SMEM_MASK = UINT16_C(0xFF80);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ST_LK_SMEM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 334);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_ST_LK_SMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == ST_LK_SMEM_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == ST_LK_SMEM_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_EXTENSION16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x007F));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & ST_LK_SMEM_MASK) == ST_LK_SMEM_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_ST_LK_SMEM) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 128U);
}

static void
test_row_boundaries(void)
{
    /* 0x767F is the last direct word; 0x7680 belongs to
     * ST_IMMEDIATE_SMEM; 0x75FF belongs to PORTW_SMEM. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x7600)) == TI_C54X_OPCODE_ID_ST_LK_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x767F)) == TI_C54X_OPCODE_ID_ST_LK_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x7680)) ==
                       TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x75FF)) ==
                       TI_C54X_OPCODE_ID_PORTW_SMEM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
