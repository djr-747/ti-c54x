/*
 * Module contract
 *
 * Owns: executable proof of the LD #K,dst catalog row (syntax 6),
 * including its exact masked identity, D/K8 population, dedicated
 * accumulator-immediate layout, empty input resources and
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

static const uint16_t LD_K_VALUE = UINT16_C(0xE800);
static const uint16_t LD_K_SAMPLE_WORD = UINT16_C(0xE8F8);
static const uint16_t LD_K_MASK = UINT16_C(0xFE00);

static bool
matches_ld_k(uint16_t word)
{
    return (word & LD_K_MASK) == LD_K_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_K);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 163);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LD_K);
    TI_C54X_TEST_CHECK(metadata->first_word_value == LD_K_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == LD_K_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_IMMEDIATE_K8);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LD_K_SAMPLE_WORD) == TI_C54X_OPCODE_ID_LD_K);

    /* Destination is bit 8, the unsigned immediate fills bits 7:0; all
     * other first-word bits are fixed. */
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x01FF));
}

static void
test_exact_population_and_disjointness(void)
{
    unsigned encoded_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool matches = matches_ld_k(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_LD_K) == matches);
        if (matches) {
            ++encoded_count;
        }
    }

    TI_C54X_TEST_CHECK(encoded_count == 512U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LD_K_VALUE) == TI_C54X_OPCODE_ID_LD_K);
}

int main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    return 0;
}
