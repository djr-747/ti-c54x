/*
 * Module contract
 *
 * Owns: executable proof that the LDM_SMEM_DIRECT catalog row
 * carries the 0x4800 value under the 0xFE80 mask with one program
 * word, the shared select/indirect/addr7 layout, no inputs, and
 * unresolved repeatability, covering exactly the 256 direct
 * first-word encodings with no shadow word. The I-set half of the
 * LDM first-word family holds no MOD rows yet and stays unbound.
 * Does not own: catalog generation, decode, memory addressing or
 * buses, accumulator/status state, CPU sequencing, timing, or
 * handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t LDM_SMEM_DIRECT_VALUE = UINT16_C(0x4800);
static const uint16_t LDM_SMEM_DIRECT_MASK = UINT16_C(0xFE80);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 340);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(metadata->first_word_value == LDM_SMEM_DIRECT_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == LDM_SMEM_DIRECT_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SELECT_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x017F));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & LDM_SMEM_DIRECT_MASK) ==
            LDM_SMEM_DIRECT_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 256U);
}

static void
test_row_boundaries(void)
{
    /* 0x4800 and 0x497F are the direct slice ends (D=0/addr=0 and
     * D=1/addr=0x7F); 0x47FF is unbound; the I-set half (0x4880,
     * 0x4980) holds no LDM MOD rows yet and stays unbound. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x4800)) ==
                       TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x4900)) ==
                       TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x497F)) ==
                       TI_C54X_OPCODE_ID_LDM_SMEM_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x47FF)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x4880)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x4980)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
