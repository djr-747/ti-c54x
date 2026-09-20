/*
 * Module contract
 *
 * Owns: executable proof that the MAR_SMEM catalog row
 * carries the 0x6D00 / 0xFF00 mask with one program
 * word, the Smem-byte layout, auxiliary-register input
 * resources, and evidenced UNRESOLVED repeatability
 * (no repeat evidence), covering exactly the 0x6D00
 * through 0x6DFF first-word range with no shadow into
 * BANZ/BANZD.
 * Does not own: catalog generation, decode, auxiliary
 * state, CPU sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MAR_SMEM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 345);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_MAR_SMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0x6D00));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFF00));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_MAR_SMEM8);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
}

static void
test_masked_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool in_range =
            word >= UINT16_C(0x6D00) && word <= UINT16_C(0x6DFF);

        if (in_range) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_MAR_SMEM) ==
                           in_range);
    }
    TI_C54X_TEST_CHECK(matches == 256U);
}

static void
test_row_boundaries(void)
{
    /* 0x6D8A is the evidenced cookie; the BANZ/BANZD
     * neighbors keep their words. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x6D8A)) ==
                       TI_C54X_OPCODE_ID_MAR_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x6CFF)) ==
                       TI_C54X_OPCODE_ID_BANZ);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x6E00)) ==
                       TI_C54X_OPCODE_ID_BANZD);
}

int
main(void)
{
    test_metadata();
    test_masked_population();
    test_row_boundaries();
    return 0;
}
