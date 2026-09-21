/*
 * Module contract
 *
 * Owns: executable proof that the STM catalog row carries the
 * 0x7700 value under the 0xFF00 mask with two program words, the
 * 2-word direct/indirect-flag/address/immediate layout, no
 * inputs, and unresolved repeatability, covering all 256
 * first-word encodings with no shadow word.
 * Does not own: catalog generation, decode, address resolution,
 * MMR routing, CPU/state mutation, fetch or bus behavior,
 * sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t STM_VALUE = UINT16_C(0x7700);
static const uint16_t STM_MASK = UINT16_C(0xFF00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_STM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 358);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_STM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == STM_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == STM_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DIRECT7_EXTENSION16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x00FF));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    unsigned stm_words = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & STM_MASK) == STM_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_STM) == matches_row);
        if (matches_row) {
            ++stm_words;
        }
    }
    TI_C54X_TEST_CHECK(matches == 256U);
    TI_C54X_TEST_CHECK(stm_words == 256U);
}

static void
test_row_boundaries(void)
{
    /* The 0x76xx ST-immediate row ends below; 0x7800 starts above. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x76FF)) != TI_C54X_OPCODE_ID_STM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x7800)) != TI_C54X_OPCODE_ID_STM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x7700)) == TI_C54X_OPCODE_ID_STM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x77FF)) == TI_C54X_OPCODE_ID_STM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
