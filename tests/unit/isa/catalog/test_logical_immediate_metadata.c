/*
 * Module contract
 *
 * Owns: executable proof of the six two-word logical-immediate
 * catalog rows, their exact masked identity, row cardinality, disjointness,
 * accumulator input declaration, and unresolved repeatability classification.
 * Does not own: operand extraction, logical semantics, CPU/state mutation,
 * sequencing, timing, generated-artifact production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct logical_immediate_row {
    ti_c54x_opcode_id id;
    uint16_t value;
    uint16_t mask;
    unsigned expected_population;
} logical_immediate_row;

static const logical_immediate_row rows[] = {
    {TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT, UINT16_C(0xF030),
     UINT16_C(0xFCF0), 64U},
    {TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT16, UINT16_C(0xF063),
     UINT16_C(0xFCFF), 4U},
    {TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT, UINT16_C(0xF040),
     UINT16_C(0xFCF0), 64U},
    {TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT16, UINT16_C(0xF064),
     UINT16_C(0xFCFF), 4U},
    {TI_C54X_OPCODE_ID_XOR_IMMEDIATE_SHIFT, UINT16_C(0xF050),
     UINT16_C(0xFCF0), 64U},
    {TI_C54X_OPCODE_ID_XOR_IMMEDIATE_SHIFT16, UINT16_C(0xF065),
     UINT16_C(0xFCFF), 4U},
};

static bool
matches(const logical_immediate_row *row, uint16_t word)
{
    return (word & row->mask) == row->value;
}

static void
test_metadata_rows(void)
{
    TI_C54X_TEST_CHECK(sizeof(rows) / sizeof(rows[0]) == 6U);

    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const logical_immediate_row *row = &rows[index];
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(row->id);

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == row->id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == row->value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == row->mask);
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
        TI_C54X_TEST_CHECK(
            metadata->operand_layout ==
                ((row->mask == UINT16_C(0xFCF0))
                     ? TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16
                     : TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16));
        TI_C54X_TEST_CHECK(metadata->repeatability ==
                           TI_C54X_REPEATABILITY_UNRESOLVED);
        TI_C54X_TEST_CHECK(metadata->input_resources ==
                           TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    }
}

static void
test_patterns_are_pairwise_disjoint(void)
{
    for (size_t left = 0U; left < sizeof(rows) / sizeof(rows[0]); ++left) {
        for (size_t right = left + 1U;
             right < sizeof(rows) / sizeof(rows[0]); ++right) {
            const logical_immediate_row *a = &rows[left];
            const logical_immediate_row *b = &rows[right];

            TI_C54X_TEST_CHECK((a->value & b->mask) !=
                               (b->value & a->mask));
        }
    }
}

static void
test_exhaustive_population_and_lookup(void)
{
    unsigned total_population = 0U;

    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const logical_immediate_row *row = &rows[index];
        unsigned population = 0U;

        for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
            const uint16_t word = (uint16_t)raw;
            const ti_c54x_opcode_id id =
                ti_c54x_opcode_catalog_lookup_first_word(word);

            if (matches(row, word)) {
                ++population;
                ++total_population;
                TI_C54X_TEST_CHECK(id == row->id);
            } else {
                TI_C54X_TEST_CHECK(id != row->id);
            }
        }
        TI_C54X_TEST_CHECK(population == row->expected_population);
    }

    TI_C54X_TEST_CHECK(total_population == 204U);
}

static void
test_fixed_bit_near_misses_are_not_immediate_rows(void)
{
    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const logical_immediate_row *row = &rows[index];
        const uint16_t fixed_bits = row->mask;

        for (unsigned bit = 0U; bit < 16U; ++bit) {
            const uint16_t bit_mask = (uint16_t)(UINT16_C(1) << bit);
            const uint16_t near_miss = (uint16_t)(row->value ^ bit_mask);

            if ((fixed_bits & bit_mask) != 0U) {
                TI_C54X_TEST_CHECK(!matches(row, near_miss));
                TI_C54X_TEST_CHECK(
                    ti_c54x_opcode_catalog_lookup_first_word(near_miss) !=
                    row->id);
            }
        }
    }
}

int
main(void)
{
    test_metadata_rows();
    test_patterns_are_pairwise_disjoint();
    test_exhaustive_population_and_lookup();
    test_fixed_bit_near_misses_are_not_immediate_rows();
    return 0;
}
