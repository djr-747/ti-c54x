/*
 * Module contract
 *
 * Owns: executable proof that the BIT_XMEM catalog row carries
 * the 0x9600 value under the 0xFF00 mask with one program word,
 * the dual-XMOD/XAR/BITC layout, auxiliary-register input, and
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

static const uint16_t BIT_XMEM_VALUE = UINT16_C(0x9600);
static const uint16_t BIT_XMEM_MASK = UINT16_C(0xFF00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_BIT_XMEM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 364);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_BIT_XMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == BIT_XMEM_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == BIT_XMEM_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_BITC);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
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
            (uint16_t)(word & BIT_XMEM_MASK) == BIT_XMEM_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_BIT_XMEM) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 256U);
}

static void
test_row_boundaries(void)
{
    /* 0x95FF belongs to LD_XMEM_SHIFT; 0x9700 is unclaimed. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x95FF)) ==
                       TI_C54X_OPCODE_ID_LD_XMEM_SHIFT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x9700)) == TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x9600)) == TI_C54X_OPCODE_ID_BIT_XMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x96FF)) == TI_C54X_OPCODE_ID_BIT_XMEM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
