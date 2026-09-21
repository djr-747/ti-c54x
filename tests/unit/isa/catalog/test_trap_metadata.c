/*
 * Module contract
 *
 * Owns: executable proof that the TRAP K catalog row carries
 * the 0xF4C0 value under the 0xFFE0 mask with one program
 * word, the TRAP_IMMEDIATE_K layout, STACK_POINTER inputs,
 * and ineligible repeatability, covering all 32 first-word
 * encodings with UNKNOWN neighbors.
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
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_TRAP_K);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 357);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_TRAP_K);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF4C0));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFFE0));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_TRAP_IMMEDIATE_K);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
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
            (uint16_t)(word & UINT16_C(0xFFE0)) == UINT16_C(0xF4C0);
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            TI_C54X_TEST_CHECK(id == TI_C54X_OPCODE_ID_TRAP_K);
            matches++;
        } else if (id == TI_C54X_OPCODE_ID_TRAP_K) {
            TI_C54X_TEST_CHECK(false);
        }
    }
    TI_C54X_TEST_CHECK(matches == 32U);
}

static void
test_row_boundaries(void)
{
    /* Both row edges decode to TRAP; the word above is unclaimed. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4C0)) == TI_C54X_OPCODE_ID_TRAP_K);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4DF)) == TI_C54X_OPCODE_ID_TRAP_K);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF4E0)) ==
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
