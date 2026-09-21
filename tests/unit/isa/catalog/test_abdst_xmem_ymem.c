/*
 * Module contract
 *
 * Owns: executable proof that the ABDST_XMEM_YMEM catalog row carries
 * the 0xE300 value under the 0xFF00 mask with one program word,
 * the dual-XMOD/XAR/YMOD/YAR layout, accumulators, ARs, SXM, FRCT, OVM input, and
 * eligible repeatability, covering all 256 first-word encodings
 * with no shadow word.
 * Does not own: catalog generation, decode, memory addressing or
 * buses, status state, CPU sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t ABDST_XMEM_YMEM_VALUE = UINT16_C(0xE300);
static const uint16_t ABDST_XMEM_YMEM_MASK = UINT16_C(0xFF00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ABDST_XMEM_YMEM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 359);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_ABDST_XMEM_YMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == ABDST_XMEM_YMEM_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == ABDST_XMEM_YMEM_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_YMOD_YAR);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_STATUS_SXM_FRCT_OVM);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x00FF));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & ABDST_XMEM_YMEM_MASK) == ABDST_XMEM_YMEM_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_ABDST_XMEM_YMEM) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 256U);
}

static void
test_row_boundaries(void)
{
    /* 0xE2FF is SQDST; 0xE400 is unclaimed. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xE2FF)) ==
                       TI_C54X_OPCODE_ID_SQDST_XMEM_YMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xE400)) == TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xE300)) == TI_C54X_OPCODE_ID_ABDST_XMEM_YMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xE3FF)) == TI_C54X_OPCODE_ID_ABDST_XMEM_YMEM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
