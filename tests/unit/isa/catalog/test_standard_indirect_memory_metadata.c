/*
 * Module contract
 *
 * Owns: executable proof of the ten neutral standard-indirect MOD0--MOD3
 * memory catalog rows, their exact masked identity, operand/resource metadata,
 * row cardinality, and direct first-word population. Does not own: decode
 * extraction, address resolution, memory effects, CPU/state mutation,
 * sequencing, timing, generated-artifact production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct standard_indirect_row {
    ti_c54x_opcode_id id;
    uint16_t value;
    uint16_t mask;
    ti_c54x_operand_layout_id operand_layout;
    ti_c54x_input_resource_mask input_resources;
    uint8_t program_word_count;
    unsigned expected_encoding_count;
} standard_indirect_row;

static const standard_indirect_row rows[] = {
    {TI_C54X_OPCODE_ID_LD_SMEM, UINT16_C(0x1080), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, UINT8_C(1), 64U},
    {TI_C54X_OPCODE_ID_LD_SMEM_TS, UINT16_C(0x1480), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, UINT8_C(1), 64U},
    {TI_C54X_OPCODE_ID_STL_SMEM, UINT16_C(0x8080), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS, UINT8_C(1), 64U},
    {TI_C54X_OPCODE_ID_LDU_SMEM, UINT16_C(0x1280), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD,
     TI_C54X_INPUT_RESOURCES_NONE, UINT8_C(1), 64U},
    {TI_C54X_OPCODE_ID_LD_T_SMEM, UINT16_C(0x3080), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD, TI_C54X_INPUT_RESOURCES_NONE,
     UINT8_C(1), 32U},
    {TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM, UINT16_C(0x7680), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD,
     TI_C54X_INPUT_RESOURCES_NONE, UINT8_C(2), 32U},
    {TI_C54X_OPCODE_ID_LD_DP_SMEM, UINT16_C(0x4680), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD, TI_C54X_INPUT_RESOURCES_NONE,
     UINT8_C(1), 32U},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM, UINT16_C(0x3280), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD, TI_C54X_INPUT_RESOURCES_NONE,
     UINT8_C(1), 32U},
    {TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16, UINT16_C(0x4480), UINT16_C(0xFEE0),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, UINT8_C(1), 64U},
    {TI_C54X_OPCODE_ID_ST_T_SMEM, UINT16_C(0x8C80), UINT16_C(0xFFE0),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD, TI_C54X_INPUT_RESOURCES_T_REGISTER,
     UINT8_C(1), 32U},
};

static bool
matches(const standard_indirect_row *row, uint16_t word)
{
    return (word & row->mask) == row->value;
}

static bool
is_expected_id(ti_c54x_opcode_id id)
{
    const size_t row_count = sizeof(rows) / sizeof(rows[0]);

    for (size_t index = 0U; index < row_count; ++index) {
        if (rows[index].id == id) {
            return true;
        }
    }
    return false;
}

static const standard_indirect_row *
row_for_id(ti_c54x_opcode_id id)
{
    const size_t row_count = sizeof(rows) / sizeof(rows[0]);

    for (size_t index = 0U; index < row_count; ++index) {
        if (rows[index].id == id) {
            return &rows[index];
        }
    }
    return NULL;
}

static void
test_metadata_rows(void)
{
    const size_t row_count = sizeof(rows) / sizeof(rows[0]);

    TI_C54X_TEST_CHECK(row_count == 10U);
    /* Unknown plus the ten neutral rows and all prior rows remains 36 IDs. */
    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 299);

    for (size_t index = 0U; index < row_count; ++index) {
        const standard_indirect_row *row = &rows[index];
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(row->id);

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == row->id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == row->value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == row->mask);
        TI_C54X_TEST_CHECK(metadata->program_word_count ==
                           row->program_word_count);
        TI_C54X_TEST_CHECK(metadata->operand_layout == row->operand_layout);
        TI_C54X_TEST_CHECK(metadata->input_resources == row->input_resources);
        TI_C54X_TEST_CHECK(metadata->repeatability ==
                           (row->id == TI_C54X_OPCODE_ID_LD_DP_SMEM
                                ? TI_C54X_REPEATABILITY_INELIGIBLE
                                : TI_C54X_REPEATABILITY_UNRESOLVED));
        TI_C54X_TEST_CHECK((uint16_t)~row->mask ==
                           (row->mask == UINT16_C(0xFEE0)
                                ? UINT16_C(0x011F)
                                : UINT16_C(0x001F)));
    }
}

static void
test_patterns_are_disjoint(void)
{
    const size_t row_count = sizeof(rows) / sizeof(rows[0]);

    for (size_t left = 0U; left < row_count; ++left) {
        for (size_t right = left + 1U; right < row_count; ++right) {
            const standard_indirect_row *a = &rows[left];
            const standard_indirect_row *b = &rows[right];
            TI_C54X_TEST_CHECK((a->value & b->mask) !=
                               (b->value & a->mask));
        }
    }
}

static void
test_modes_arf_and_d_population(void)
{
    unsigned populated_first_words = 0U;
    unsigned expected_total = 0U;
    const size_t row_count = sizeof(rows) / sizeof(rows[0]);

    for (size_t index = 0U; index < row_count; ++index) {
        expected_total += rows[index].expected_encoding_count;
    }

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (id != TI_C54X_OPCODE_ID_UNKNOWN) {
            ++populated_first_words;
        }

        if (is_expected_id(id)) {
            const standard_indirect_row *row = row_for_id(id);
            TI_C54X_TEST_CHECK(row != NULL);
            TI_C54X_TEST_CHECK(matches(row, word));
        }
    }
    /* W42B's 1863 words plus 240 indexed and 240 circular words. */
    TI_C54X_TEST_CHECK(expected_total == 480U);
    TI_C54X_TEST_CHECK(populated_first_words == 33801U);

    for (size_t index = 0U; index < row_count; ++index) {
        const standard_indirect_row *row = &rows[index];
        unsigned encoded_count = 0U;

        for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
            const uint16_t word = (uint16_t)raw;
            if (matches(row, word)) {
                ++encoded_count;
                TI_C54X_TEST_CHECK(
                    ti_c54x_opcode_catalog_lookup_first_word(word) ==
                    row->id);
            }
        }
        TI_C54X_TEST_CHECK(encoded_count == row->expected_encoding_count);
    }
}

int
main(void)
{
    test_metadata_rows();
    test_patterns_are_disjoint();
    test_modes_arf_and_d_population();
    return 0;
}
