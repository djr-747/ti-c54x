/*
 * Module contract
 *
 * Owns: executable proof of the CALL pmad catalog row (Table 2-13
 * syntax 2), including its exact masked identity, single-encoding
 * population, reused branch-pmad layout, STACK_POINTER input
 * resources and INELIGIBLE repeatability metadata, and fixed-bit
 * separation.
 * Does not own: opcode decoding, operand extraction, stack pushes,
 * CPU/state mutation, sequencing, timing, generated lookup production,
 * or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t CALL_PMAD_VALUE = UINT16_C(0xF074);
static const uint16_t CALL_PMAD_MASK = UINT16_C(0xFFFF);
/* Bit 9 set selects the CALLD sibling row bound in W67C. */
static const uint16_t CALL_DELAYED_WORD = UINT16_C(0xF274);

static bool
matches_call_pmad(uint16_t word)
{
    return (word & CALL_PMAD_MASK) == CALL_PMAD_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_CALL_PMAD);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 205);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_CALL_PMAD);
    TI_C54X_TEST_CHECK(metadata->first_word_value == CALL_PMAD_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == CALL_PMAD_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           CALL_PMAD_VALUE) == TI_C54X_OPCODE_ID_CALL_PMAD);

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
        const bool matches = matches_call_pmad(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_CALL_PMAD) == matches);
        if (matches) {
            ++encoded_count;
        }
    }

    TI_C54X_TEST_CHECK(encoded_count == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           CALL_DELAYED_WORD) != TI_C54X_OPCODE_ID_CALL_PMAD);
}

int main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    return 0;
}
