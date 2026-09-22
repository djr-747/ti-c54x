/*
 * Module contract
 *
 * Owns: executable proof that the POLY_SMEM catalog row carries the
 * 0x3600 value under the 0xFF00 mask with one program word, the
 * shared indirect/addr7 layout, accumulator/T/SXM/FRCT/OVM
 * inputs, and eligible repeatability, covering all 256 first-word
 * encodings with the MACA row on both edges.
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

static const uint16_t POLY_SMEM_VALUE = UINT16_C(0x3600);
static const uint16_t POLY_SMEM_MASK = UINT16_C(0xFF00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_POLY_SMEM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 379);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_POLY_SMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == POLY_SMEM_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == POLY_SMEM_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_T_REGISTER_STATUS_SXM_FRCT_OVM);
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
            (uint16_t)(word & POLY_SMEM_MASK) == POLY_SMEM_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_POLY_SMEM) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 256U);
}

static void
test_row_boundaries(void)
{
    /* The POLY row sits inside the MACA split range: 0x35FF and
     * 0x3700 both decode to MACA_SMEM. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x35FF)) == TI_C54X_OPCODE_ID_MACA_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x3600)) == TI_C54X_OPCODE_ID_POLY_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x36FF)) == TI_C54X_OPCODE_ID_POLY_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x3700)) == TI_C54X_OPCODE_ID_MACA_SMEM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
