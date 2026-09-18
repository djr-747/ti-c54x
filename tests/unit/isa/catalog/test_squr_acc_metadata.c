/*
 * Module contract
 *
 * Owns: executable proof that the SQURA_SMEM and SQURS_SMEM catalog
 * rows carry the 0x3800/0x3A00 values under the 0xFE00 mask with one
 * program word each, the shared destination/indirect/addr7 layout,
 * accumulator/T/FRCT/OVM inputs, and eligible repeatability,
 * covering all 512 first-word encodings per row with no shadow
 * word.
 * Does not own: catalog generation, decode, accumulator/T/status
 * state, CPU sequencing, fetch or bus behavior, timing, or handler
 * bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct squr_acc_row {
    ti_c54x_opcode_id id;
    uint16_t value;
} squr_acc_row;

static const squr_acc_row rows[] = {
    {TI_C54X_OPCODE_ID_SQURA_SMEM, UINT16_C(0x3800)},
    {TI_C54X_OPCODE_ID_SQURS_SMEM, UINT16_C(0x3A00)},
};
static const uint16_t SQUR_ACC_MASK = UINT16_C(0xFE00);

static void
test_metadata(void)
{
    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 292);
    for (size_t i = 0U; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(rows[i].id);

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == rows[i].id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == rows[i].value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == SQUR_ACC_MASK);
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
        TI_C54X_TEST_CHECK(metadata->operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7);
        TI_C54X_TEST_CHECK(metadata->repeatability ==
                           TI_C54X_REPEATABILITY_ELIGIBLE);
        TI_C54X_TEST_CHECK(metadata->input_resources ==
                           TI_C54X_INPUT_RESOURCES_ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM);
        TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                           UINT16_C(0x01FF));
    }
}

static void
test_exact_population(void)
{
    for (size_t i = 0U; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        unsigned matches = 0U;
        uint32_t raw;

        for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
            const uint16_t word = (uint16_t)raw;
            const bool matches_row =
                (uint16_t)(word & SQUR_ACC_MASK) == rows[i].value;
            ti_c54x_opcode_id id =
                ti_c54x_opcode_catalog_lookup_first_word(word);

            if (matches_row) {
                ++matches;
            }
            TI_C54X_TEST_CHECK((id == rows[i].id) == matches_row);
        }
        TI_C54X_TEST_CHECK(matches == 512U);
    }
}

static void
test_row_boundaries(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x37FF)) != TI_C54X_OPCODE_ID_SQURA_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x3C00)) != TI_C54X_OPCODE_ID_SQURS_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x3800)) == TI_C54X_OPCODE_ID_SQURA_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x39FF)) == TI_C54X_OPCODE_ID_SQURA_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x3A00)) == TI_C54X_OPCODE_ID_SQURS_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x3BFF)) == TI_C54X_OPCODE_ID_SQURS_SMEM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
