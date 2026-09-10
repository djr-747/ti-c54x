/*
 * Module contract
 *
 * Owns: executable proof of the LD #lk,16,dst catalog row (syntax 8),
 * including its exact masked identity, D/lk16 population, dedicated
 * accumulator-immediate layout, SXM input resources and
 * repeatability metadata, and fixed-bit separation.
 * Does not own: opcode decoding, operand extraction, accumulator reads,
 * CPU/state mutation, sequencing, timing, generated lookup production,
 * or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t LD_LK16_VALUE = UINT16_C(0xF062);
static const uint16_t LD_LK16_SAMPLE_WORD = UINT16_C(0xF162);
static const uint16_t LD_LK16_MASK = UINT16_C(0xFEFF);

static bool
matches_ld_lk16(uint16_t word)
{
    return (word & LD_LK16_MASK) == LD_LK16_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_LK_16);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 141);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LD_LK_16);
    TI_C54X_TEST_CHECK(metadata->first_word_value == LD_LK16_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == LD_LK16_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_LK16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LD_LK16_SAMPLE_WORD) == TI_C54X_OPCODE_ID_LD_LK_16);

    /* Destination is bit 8; every other first-word bit is fixed. */
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x0100));
}

static void
test_exact_population_and_disjointness(void)
{
    unsigned encoded_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool matches = matches_ld_lk16(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_LD_LK_16) == matches);
        if (matches) {
            ++encoded_count;
        }
    }

    TI_C54X_TEST_CHECK(encoded_count == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LD_LK16_VALUE) == TI_C54X_OPCODE_ID_LD_LK_16);
}

int main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    return 0;
}
