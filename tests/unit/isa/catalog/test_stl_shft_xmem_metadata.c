/*
 * Module contract
 *
 * Owns: executable proof that the STL_SHFT_XMEM catalog row
 * carries the 0x9800 value under the 0xFE00 mask with one program
 * word, the shared accumulator/dual-XMOD/XAR/shift-4 layout,
 * accumulator-plus-auxiliary inputs, and unresolved repeatability,
 * covering exactly the 512 Xmem first-word encodings with the
 * 0x97FF UNKNOWN lower sibling and the 0x9A00 STH upper sibling
 * pinned.
 * Does not own: catalog generation, decode, memory addressing or
 * buses, accumulator/status state, CPU sequencing, timing, or
 * handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t STL_SHFT_XMEM_VALUE = UINT16_C(0x9800);
static const uint16_t STL_SHFT_XMEM_MASK = UINT16_C(0xFE00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_STL_SHFT_XMEM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 332);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_STL_SHFT_XMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == STL_SHFT_XMEM_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == STL_SHFT_XMEM_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_DUAL_XMOD_XAR_SHIFT4);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x01FF));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & STL_SHFT_XMEM_MASK) ==
            STL_SHFT_XMEM_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_STL_SHFT_XMEM) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 512U);
}

static void
test_row_boundaries(void)
{
    /* 0x9800 and 0x99FF are the Xmem slice ends; 0x97FF decodes
     * UNKNOWN below; 0x9A00 opens the established STH Xmem row. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x9800)) ==
                       TI_C54X_OPCODE_ID_STL_SHFT_XMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x99FF)) ==
                       TI_C54X_OPCODE_ID_STL_SHFT_XMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x97FF)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x9A00)) ==
                       TI_C54X_OPCODE_ID_STH_SHFT_XMEM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
