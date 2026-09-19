/*
 * Module contract
 *
 * Owns: executable metadata and generated-lookup proofs for the three
 * neutral ANDM/ORM/XORM WORD16 memory catalog rows plus the ADDM row,
 * which shares their shape with OVM/SXM modes. Does not own: decode
 * extraction, address arithmetic, semantic effects, CPU transactions,
 * inventory policy, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

typedef struct atomic_row {
    ti_c54x_opcode_id id;
    uint16_t value;
    ti_c54x_input_resource_mask resources;
} atomic_row;

static const atomic_row rows[] = {
    {TI_C54X_OPCODE_ID_ANDM_SMEM_MOD0_3, UINT16_C(0x6880),
     TI_C54X_INPUT_RESOURCES_NONE},
    {TI_C54X_OPCODE_ID_ORM_SMEM_MOD0_3, UINT16_C(0x6980),
     TI_C54X_INPUT_RESOURCES_NONE},
    {TI_C54X_OPCODE_ID_XORM_SMEM_MOD0_3, UINT16_C(0x6A80),
     TI_C54X_INPUT_RESOURCES_NONE},
    {TI_C54X_OPCODE_ID_ADDM_SMEM_MOD0_3, UINT16_C(0x6B80),
     TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM},
};

static void
test_rows_and_projection(void)
{
    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 317);
    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const atomic_row *row = &rows[index];
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(row->id);
        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == row->id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == row->value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFFE0));
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
        TI_C54X_TEST_CHECK(metadata->operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD);
        TI_C54X_TEST_CHECK(metadata->repeatability ==
                           TI_C54X_REPEATABILITY_INELIGIBLE);
        TI_C54X_TEST_CHECK(metadata->input_resources == row->resources);
    }
}

static void
test_lookup_population_and_disjoint_rows(void)
{
    unsigned populated = 0U;
    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        if (id != TI_C54X_OPCODE_ID_UNKNOWN) {
            ++populated;
        }
        for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]);
             ++index) {
            if (id == rows[index].id) {
                TI_C54X_TEST_CHECK((word & UINT16_C(0xFFE0)) ==
                                   rows[index].value);
            }
        }
    }
    TI_C54X_TEST_CHECK(populated == 38729U);
    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        unsigned encodings = 0U;
        for (uint32_t low = 0U; low < UINT32_C(32); ++low) {
            const uint16_t word =
                (uint16_t)(rows[index].value | (uint16_t)low);
            ++encodings;
            TI_C54X_TEST_CHECK(
                ti_c54x_opcode_catalog_lookup_first_word(word) == rows[index].id);
        }
        TI_C54X_TEST_CHECK(encodings == 32U);
    }
}

int
main(void)
{
    test_rows_and_projection();
    test_lookup_population_and_disjoint_rows();
    return 0;
}
