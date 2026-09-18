/*
 * Module contract
 *
 * Owns: executable proof of the W47B2 long32 DLD/DST catalog rows, including
 * masked identity, input-resource declarations, repeatability, and all
 * sixteen first-word encodings per row. Does not own: generated lookup
 * production, operand extraction, semantics, memory effects, or CPU state.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

typedef struct long32_row {
    ti_c54x_opcode_id id;
    uint16_t value;
    uint16_t mask;
    ti_c54x_repeatability_class repeatability;
    ti_c54x_input_resource_mask input_resources;
} long32_row;

static const long32_row rows[] = {
    {TI_C54X_OPCODE_ID_DLD_LONG32_MOD13, UINT16_C(0x56E8),
     UINT16_C(0xFEF8), TI_C54X_REPEATABILITY_UNRESOLVED,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM},
    {TI_C54X_OPCODE_ID_DST_LONG32_MOD13, UINT16_C(0x4EE8),
     UINT16_C(0xFEF8), TI_C54X_REPEATABILITY_INELIGIBLE,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_SST},
};

static void
test_metadata_rows(void)
{
    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 297);

    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const long32_row *row = &rows[index];
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(row->id);

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == row->id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == row->value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == row->mask);
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
        TI_C54X_TEST_CHECK(metadata->operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_EXTENSION16_MOD4);
        TI_C54X_TEST_CHECK(metadata->repeatability == row->repeatability);
        TI_C54X_TEST_CHECK(metadata->input_resources == row->input_resources);
        TI_C54X_TEST_CHECK((uint16_t)~row->mask == UINT16_C(0x0107));
    }
}

static void
test_all_first_word_encodings(void)
{
    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const long32_row *row = &rows[index];
        unsigned encoding_count = 0U;

        for (uint16_t ds = 0U; ds < UINT16_C(2); ++ds) {
            for (uint16_t arf = 0U; arf < UINT16_C(8); ++arf) {
                const uint16_t first_word = (uint16_t)(
                    row->value | (uint16_t)(ds << 8U) | arf);
                ++encoding_count;
                TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                                       first_word) == row->id);
            }
        }
        TI_C54X_TEST_CHECK(encoding_count == 16U);
    }
}

static void
test_fixed_bit_near_misses_do_not_bind(void)
{
    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const long32_row *row = &rows[index];
        const uint16_t near_miss = (uint16_t)(row->value ^ UINT16_C(0x0008));
        const uint16_t adjacent = (uint16_t)(row->value ^ UINT16_C(0x0080));

        TI_C54X_TEST_CHECK((near_miss & row->mask) != row->value);
        TI_C54X_TEST_CHECK((adjacent & row->mask) != row->value);
        TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                               near_miss) == TI_C54X_OPCODE_ID_UNKNOWN);
        TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                               adjacent) == TI_C54X_OPCODE_ID_UNKNOWN);
    }
}

int
main(void)
{
    test_metadata_rows();
    test_all_first_word_encodings();
    test_fixed_bit_near_misses_do_not_bind();
    return 0;
}
