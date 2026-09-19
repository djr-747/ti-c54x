/*
 * Module contract
 *
 * Owns: executable proof that the MVMM catalog row carries the
 * 0xE700 value under the 0xFF00 mask with one program word (the
 * 4-bit MMR source rides bits 7:4, the 4-bit MMR destination
 * rides bits 3:0), covering all 256 first-word encodings with
 * no shadow word.
 * Does not own: catalog generation, decode, address resolution,
 * MMR routing, CPU/state mutation, fetch or bus behavior,
 * sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t MVMM_VALUE = UINT16_C(0xE700);
static const uint16_t MVMM_MASK = UINT16_C(0xFF00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MVMM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 327);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_MVMM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == MVMM_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == MVMM_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_MMR4_MMR4);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
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
            (uint16_t)(word & MVMM_MASK) == MVMM_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            TI_C54X_TEST_CHECK(id == TI_C54X_OPCODE_ID_MVMM);
            matches++;
        } else if (id == TI_C54X_OPCODE_ID_MVMM) {
            TI_C54X_TEST_CHECK(false);
        }
    }
    TI_C54X_TEST_CHECK(matches == 256U);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    return 0;
}
