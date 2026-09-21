/*
 * Module contract
 *
 * Owns: executable proof that the LMS Xmem,Ymem catalog row
 * carries the 0xE100 value under the 0xFF00 mask with one
 * program word, the DUAL_XMOD_XAR_YMOD_YAR layout,
 * accumulator plus auxiliary-register plus SXM/FRCT/OVM
 * inputs, and eligible repeatability, covering all 256
 * first-word encodings with UNKNOWN neighbors.
 * Does not own: catalog generation, decode, memory addressing or
 * buses, status state, CPU sequencing, timing, or handler bodies.
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
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LMS_XMEM_YMEM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 358);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LMS_XMEM_YMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xE100));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFF00));
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
            (uint16_t)(word & UINT16_C(0xFF00)) == UINT16_C(0xE100);
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            TI_C54X_TEST_CHECK(id == TI_C54X_OPCODE_ID_LMS_XMEM_YMEM);
            matches++;
        } else if (id == TI_C54X_OPCODE_ID_LMS_XMEM_YMEM) {
            TI_C54X_TEST_CHECK(false);
        }
    }
    TI_C54X_TEST_CHECK(matches == 256U);
}

static void
test_row_boundaries(void)
{
    /* 0xE0FF is FIRS (P3-S108 row); 0xE200 is SQDST, the
     * upper TI neighbor. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xE0FF)) ==
                       TI_C54X_OPCODE_ID_FIRS_XMEM_YMEM_PMAD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xE100)) ==
                       TI_C54X_OPCODE_ID_LMS_XMEM_YMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xE1FF)) ==
                       TI_C54X_OPCODE_ID_LMS_XMEM_YMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xE200)) ==
                       TI_C54X_OPCODE_ID_SQDST_XMEM_YMEM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
