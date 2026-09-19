/*
 * Module contract
 *
 * Owns: executable proof of the BANZ catalog row (Table 2-12
 * syntax 3, Z=0), including its exact masked identity,
 * 256-encoding population over the Sind byte, dedicated Sind
 * layout, auxiliary-register input resources and INELIGIBLE
 * repeatability metadata, and fixed-bit separation from the
 * deferred BANZD word.
 * Does not own: opcode decoding, Sind evaluation, AR writes,
 * CPU/state mutation, sequencing, timing, generated lookup
 * production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t BANZ_VALUE = UINT16_C(0x6C00);
static const uint16_t BANZ_MASK = UINT16_C(0xFF00);
/* Bit 9 set selects the deferred delayed BANZD variant. */
static const uint16_t BANZ_DELAYED_WORD = UINT16_C(0x6E00);

static bool
matches_banz(uint16_t word)
{
    return (word & BANZ_MASK) == BANZ_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_BANZ);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 312);
    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 312);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_BANZ);
    TI_C54X_TEST_CHECK(metadata->first_word_value == BANZ_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == BANZ_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_SIND_PMAD16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           BANZ_VALUE) == TI_C54X_OPCODE_ID_BANZ);

    /* Only the 8-bit Sind field varies; Z stays fixed for BANZ. */
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x00FF));
}

static void
test_exact_population_and_disjointness(void)
{
    unsigned encoded_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool matches = matches_banz(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_BANZ) == matches);
        if (matches) {
            ++encoded_count;
        }
    }

    TI_C54X_TEST_CHECK(encoded_count == 256U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           BANZ_DELAYED_WORD) != TI_C54X_OPCODE_ID_BANZ);
}

int main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    return 0;
}
