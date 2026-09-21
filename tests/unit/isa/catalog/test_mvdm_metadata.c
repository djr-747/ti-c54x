/*
 * Module contract
 *
 * Owns: executable proof that the MVDM catalog row carries the
 * 0x7200 value under the 0xFFE0 mask with two program words (the
 * MMR destination rides the low five bits, the 16-bit absolute
 * Smem address rides the second word), covering all 32
 * first-word encodings with no shadow word, including the
 * 5110 boot-halt words 0x7214/0x7212/0x7215.
 * Does not own: catalog generation, decode, address resolution,
 * MMR routing, CPU/state mutation, fetch or bus behavior,
 * sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t MVDM_VALUE = UINT16_C(0x7200);
static const uint16_t MVDM_MASK = UINT16_C(0xFFE0);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MVDM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 351);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_MVDM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == MVDM_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == MVDM_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x001F));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & MVDM_MASK) == MVDM_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            TI_C54X_TEST_CHECK(id == TI_C54X_OPCODE_ID_MVDM);
            matches++;
        } else if (id == TI_C54X_OPCODE_ID_MVDM) {
            TI_C54X_TEST_CHECK(false);
        }
    }
    TI_C54X_TEST_CHECK(matches == 32U);
}

static void
test_boot_halt_words(void)
{
    /* P3-S56 RED: the 5110 boot halt decodes MVDM Smem,AR4/AR2/AR5. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x7214)) == TI_C54X_OPCODE_ID_MVDM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x7212)) == TI_C54X_OPCODE_ID_MVDM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x7215)) == TI_C54X_OPCODE_ID_MVDM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_boot_halt_words();
    return 0;
}
