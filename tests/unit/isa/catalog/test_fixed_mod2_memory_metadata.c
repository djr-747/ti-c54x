/*
 * Module contract
 *
 * Owns: executable proof of the three fixed-MOD2 WORD16 memory catalog rows,
 * their exact masked identity, operand/resource metadata, row cardinality,
 * and direct first-word population. Does not own: decode extraction,
 * address resolution, memory effects, CPU/state mutation, sequencing,
 * timing, generated-artifact production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct fixed_mod2_row {
    ti_c54x_opcode_id id;
    uint16_t value;
    uint16_t mask;
    ti_c54x_operand_layout_id operand_layout;
    ti_c54x_input_resource_mask input_resources;
} fixed_mod2_row;

static const fixed_mod2_row rows[] = {
    {TI_C54X_OPCODE_ID_LD_SMEM_MOD2, UINT16_C(0x1090), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM},
    {TI_C54X_OPCODE_ID_STL_SMEM_MOD2, UINT16_C(0x8090), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS},
    {TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD2, UINT16_C(0x4490),
     UINT16_C(0xFEF8), TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM},
    {TI_C54X_OPCODE_ID_ST_T_SMEM_MOD2, UINT16_C(0x8C90), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF,
     TI_C54X_INPUT_RESOURCES_T_REGISTER},
};

static bool
matches(const fixed_mod2_row *row, uint16_t word)
{
    return (word & row->mask) == row->value;
}

static void
test_metadata_rows(void)
{
    const size_t row_count = sizeof(rows) / sizeof(rows[0]);

    TI_C54X_TEST_CHECK(row_count == 4U);
    /* Unknown plus the 34 catalog rows is the complete generated identity. */
    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 35);

    for (size_t index = 0U; index < row_count; ++index) {
        const fixed_mod2_row *row = &rows[index];
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(row->id);

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == row->id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == row->value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == row->mask);
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
        TI_C54X_TEST_CHECK(metadata->operand_layout == row->operand_layout);
        TI_C54X_TEST_CHECK(metadata->input_resources == row->input_resources);
        TI_C54X_TEST_CHECK(metadata->repeatability ==
                           TI_C54X_REPEATABILITY_UNRESOLVED);

        /* Accumulator rows vary D/S and ARF; ST T varies only ARF. */
        TI_C54X_TEST_CHECK((uint16_t)~row->mask ==
                           (row->id == TI_C54X_OPCODE_ID_ST_T_SMEM_MOD2
                                ? UINT16_C(0x0007)
                                : UINT16_C(0x0107)));
    }
}

static void
test_fixed_patterns_are_disjoint(void)
{
    const size_t row_count = sizeof(rows) / sizeof(rows[0]);

    for (size_t left = 0U; left < row_count; ++left) {
        for (size_t right = left + 1U; right < row_count; ++right) {
            const fixed_mod2_row *a = &rows[left];
            const fixed_mod2_row *b = &rows[right];
            TI_C54X_TEST_CHECK((a->value & b->mask) !=
                               (b->value & a->mask));
        }
    }
}

static void
test_each_row_has_expected_encodings_and_population(void)
{
    unsigned populated_first_words = 0U;
    const size_t row_count = sizeof(rows) / sizeof(rows[0]);

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (id != TI_C54X_OPCODE_ID_UNKNOWN) {
            ++populated_first_words;
        }
    }
    TI_C54X_TEST_CHECK(populated_first_words == 1487U);

    for (size_t index = 0U; index < row_count; ++index) {
        const fixed_mod2_row *row = &rows[index];
        unsigned encoded_count = 0U;

        for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
            const uint16_t word = (uint16_t)raw;
            if (matches(row, word)) {
                ++encoded_count;
                TI_C54X_TEST_CHECK(
                    ti_c54x_opcode_catalog_lookup_first_word(word) == row->id);
            }
        }
        TI_C54X_TEST_CHECK(
            encoded_count ==
            (row->id == TI_C54X_OPCODE_ID_ST_T_SMEM_MOD2 ? 8U : 16U));
    }
}

int
main(void)
{
    test_metadata_rows();
    test_fixed_patterns_are_disjoint();
    test_each_row_has_expected_encodings_and_population();
    return 0;
}
