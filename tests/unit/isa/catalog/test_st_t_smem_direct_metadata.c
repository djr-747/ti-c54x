/*
 * Module contract
 *
 * Owns: executable proof that the ST_T_SMEM_DIRECT catalog row
 * carries the 0x8C00 value under the 0xFF80 mask with one program
 * word, the shared indirect/addr7 layout, T-register inputs, and
 * unresolved repeatability, covering exactly the 128 direct
 * first-word encodings with no shadow word and no overlap into
 * the established indirect ST_T rows.
 * Does not own: catalog generation, decode, memory addressing or
 * buses, accumulator/status state, CPU sequencing, timing, or
 * handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t ST_T_SMEM_DIRECT_VALUE = UINT16_C(0x8C00);
static const uint16_t ST_T_SMEM_DIRECT_MASK = UINT16_C(0xFF80);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 310);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(metadata->first_word_value == ST_T_SMEM_DIRECT_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == ST_T_SMEM_DIRECT_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_T_REGISTER);
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
            (uint16_t)(word & ST_T_SMEM_DIRECT_MASK) ==
            ST_T_SMEM_DIRECT_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 128U);
}

static void
test_row_boundaries(void)
{
    /* 0x8C00 and 0x8C7F are the direct slice ends; 0x8BFF is
     * the pinned POPD sibling; 0x8C80 opens the established
     * indirect ST_T rows. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x8C00)) ==
                       TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x8C7F)) ==
                       TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x8BFF)) ==
                       TI_C54X_OPCODE_ID_POPD_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x8C80)) ==
                       TI_C54X_OPCODE_ID_ST_T_SMEM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
