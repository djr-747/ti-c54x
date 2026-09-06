/*
 * Module contract
 *
 * Owns: executable proof of the planned AND/OR/XOR accumulator catalog
 * rows, including masked identity, one-word width, shared operand/resource
 * metadata, row cardinality, and direct first-word population.
 * Does not own: operand extraction, logical semantics, CPU/state mutation,
 * sequencing, timing, generated-artifact production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct logical_row {
    ti_c54x_opcode_id id;
    uint16_t value;
    uint16_t mask;
} logical_row;

static const logical_row logical_rows[] = {
    {TI_C54X_OPCODE_ID_AND_ACC_SHIFT, UINT16_C(0xF080),
     UINT16_C(0xFCE0)},
    {TI_C54X_OPCODE_ID_OR_ACC_SHIFT, UINT16_C(0xF0A0), UINT16_C(0xFCE0)},
    {TI_C54X_OPCODE_ID_XOR_ACC_SHIFT, UINT16_C(0xF0C0),
     UINT16_C(0xFCE0)},
};

static bool
matches(const logical_row *row, uint16_t word)
{
    return (word & row->mask) == row->value;
}

static void
test_metadata_rows(void)
{
    const size_t logical_row_count =
        sizeof(logical_rows) / sizeof(logical_rows[0]);

    TI_C54X_TEST_CHECK(logical_row_count == 3U);
    /* UNKNOWN is the zero identity, so COUNT - 1 is the row count. */
    TI_C54X_TEST_CHECK((unsigned)TI_C54X_OPCODE_ID_COUNT - 1U == 13U);

    for (size_t index = 0U; index < logical_row_count; ++index) {
        const logical_row *row = &logical_rows[index];
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(row->id);

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == row->id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == row->value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == row->mask);
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
        TI_C54X_TEST_CHECK(metadata->operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
        TI_C54X_TEST_CHECK(metadata->input_resources ==
                           TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
        TI_C54X_TEST_CHECK(metadata->repeatability ==
                           TI_C54X_REPEATABILITY_UNRESOLVED);
    }
}

static void
test_fixed_patterns_are_disjoint(void)
{
    const size_t row_count = sizeof(logical_rows) / sizeof(logical_rows[0]);

    for (size_t left = 0U; left < row_count; ++left) {
        for (size_t right = left + 1U; right < row_count; ++right) {
            const logical_row *a = &logical_rows[left];
            const logical_row *b = &logical_rows[right];
            TI_C54X_TEST_CHECK((a->value & b->mask) !=
                               (b->value & a->mask));
        }
    }
}

static void
test_each_row_has_128_encoded_words_and_representatives(void)
{
    const size_t row_count = sizeof(logical_rows) / sizeof(logical_rows[0]);

    for (size_t index = 0U; index < row_count; ++index) {
        const logical_row *row = &logical_rows[index];
        unsigned encoded_count = 0U;

        for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
            const uint16_t word = (uint16_t)raw;
            if (matches(row, word)) {
                ++encoded_count;
                TI_C54X_TEST_CHECK(
                    ti_c54x_opcode_catalog_lookup_first_word(word) == row->id);
            }
        }
        TI_C54X_TEST_CHECK(encoded_count == 128U);

        /* Exercise each source/destination and both signed-shift endpoints. */
        {
            const uint16_t representatives[] = {
                row->value,
                (uint16_t)(row->value | UINT16_C(0x0001)),
                (uint16_t)(row->value | UINT16_C(0x000F)),
                (uint16_t)(row->value | UINT16_C(0x0010)),
                (uint16_t)(row->value | UINT16_C(0x001F)),
                (uint16_t)(row->value | UINT16_C(0x0100)),
                (uint16_t)(row->value | UINT16_C(0x0200)),
                (uint16_t)(row->value | UINT16_C(0x031F))};
            for (size_t rep = 0U;
                 rep < sizeof(representatives) / sizeof(representatives[0]);
                 ++rep) {
                TI_C54X_TEST_CHECK(matches(row, representatives[rep]));
                TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                                       representatives[rep]) == row->id);
            }
        }
    }
}

static void
test_post_slice_direct_population(void)
{
    unsigned populated = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        if (ti_c54x_opcode_catalog_lookup_first_word((uint16_t)raw) !=
            TI_C54X_OPCODE_ID_UNKNOWN) {
            ++populated;
        }
    }
    TI_C54X_TEST_CHECK(populated == 1077U);
}

int
main(void)
{
    test_metadata_rows();
    test_fixed_patterns_are_disjoint();
    test_each_row_has_128_encoded_words_and_representatives();
    test_post_slice_direct_population();
    return 0;
}
