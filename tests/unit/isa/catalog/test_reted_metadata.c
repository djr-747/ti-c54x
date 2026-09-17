/*
 * Module contract
 *
 * Owns: executable proof of the RETED catalog row (Table 2-15
 * syntax 5, Z=1), including its exact masked identity,
 * single-encoding population, reused no-operand layout,
 * STACK_POINTER input resources and INELIGIBLE repeatability
 * metadata, and fixed-bit separation from the RETE row.
 * Does not own: opcode decoding, operand extraction, stack pops,
 * CPU/state mutation, sequencing, timing, generated lookup production,
 * or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t RETED_VALUE = UINT16_C(0xF6EB);
static const uint16_t RETED_MASK = UINT16_C(0xFFFF);
/* Bit 9 clear selects the non-delayed RETE sibling. */
static const uint16_t RETE_WORD = UINT16_C(0xF4EB);

static bool
matches_reted(uint16_t word)
{
    return (word & RETED_MASK) == RETED_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_RETED);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 262);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_RETED);
    TI_C54X_TEST_CHECK(metadata->first_word_value == RETED_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == RETED_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           RETED_VALUE) == TI_C54X_OPCODE_ID_RETED);

    /* Every first-word bit is fixed; the mask admits no operand bits. */
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x0000));
}

static void
test_exact_population_and_disjointness(void)
{
    unsigned encoded_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool matches = matches_reted(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_RETED) == matches);
        if (matches) {
            ++encoded_count;
        }
    }

    TI_C54X_TEST_CHECK(encoded_count == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           RETE_WORD) != TI_C54X_OPCODE_ID_RETED);
}

int main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    return 0;
}
