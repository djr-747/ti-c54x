/*
 * Module contract
 *
 * Owns: executable proof of the planned EXP_ACC catalog row, including its
 * masked first-word identity, one-word width, shared accumulator-source
 * operand layout, accumulator input declaration, and unresolved repeatability
 * classification.
 * Does not own: operand extraction, EXP arithmetic, T-register effects, CPU
 * state mutation, sequencing, timing, reset-runnable scenario accounting,
 * generated-artifact production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

static void
test_exp_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_EXP_ACC);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_EXP_ACC);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0xF48E));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFEFF));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    /* The source's omission from the nonrepeatable list is not eligibility. */
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
}

static void
test_exp_first_word_population(void)
{
    unsigned matches = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const int matches_exp =
            (word & UINT16_C(0xFEFF)) == UINT16_C(0xF48E);
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_exp) {
            ++matches;
            TI_C54X_TEST_CHECK(id == TI_C54X_OPCODE_ID_EXP_ACC);
        } else {
            TI_C54X_TEST_CHECK(id != TI_C54X_OPCODE_ID_EXP_ACC);
        }
    }

    /* Only selector S (bit 8) varies in the EXP masked row. */
    TI_C54X_TEST_CHECK(matches == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF48E)) ==
                       TI_C54X_OPCODE_ID_EXP_ACC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF58E)) ==
                       TI_C54X_OPCODE_ID_EXP_ACC);
}

int
main(void)
{
    test_exp_metadata();
    test_exp_first_word_population();
    return 0;
}
