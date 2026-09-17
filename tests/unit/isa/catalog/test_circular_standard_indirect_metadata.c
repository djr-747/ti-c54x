/*
 * Module contract
 *
 * Owns: executable metadata and generated-lookup proofs for the twenty
 * private CMPT=0 circular MOD8/MOD10 standard-indirect catalog rows.
 * Does not own: decode extraction, address arithmetic, semantic effects, CPU
 * transactions, or inventory quality policy.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

typedef struct circular_row {
    ti_c54x_opcode_id id;
    uint16_t value;
    uint16_t mask;
    ti_c54x_operand_layout_id layout;
    ti_c54x_input_resource_mask resources;
    uint8_t words;
    unsigned encodings;
} circular_row;

static const circular_row rows[] = {
    {TI_C54X_OPCODE_ID_LD_SMEM_MOD8, UINT16_C(0x10C0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U, 16U},
    {TI_C54X_OPCODE_ID_LD_SMEM_MOD10, UINT16_C(0x10D0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U, 16U},
    {TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD8, UINT16_C(0x14C0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, 1U, 16U},
    {TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD10, UINT16_C(0x14D0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, 1U, 16U},
    {TI_C54X_OPCODE_ID_STL_SMEM_MOD8, UINT16_C(0x80C0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS, 1U, 16U},
    {TI_C54X_OPCODE_ID_STL_SMEM_MOD10, UINT16_C(0x80D0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS, 1U, 16U},
    {TI_C54X_OPCODE_ID_LDU_SMEM_MOD8, UINT16_C(0x12C0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 1U, 16U},
    {TI_C54X_OPCODE_ID_LDU_SMEM_MOD10, UINT16_C(0x12D0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 1U, 16U},
    {TI_C54X_OPCODE_ID_LD_T_SMEM_MOD8, UINT16_C(0x30C0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_LD_T_SMEM_MOD10, UINT16_C(0x30D0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD8, UINT16_C(0x76C0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 2U, 8U},
    {TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD10, UINT16_C(0x76D0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 2U, 8U},
    {TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD8, UINT16_C(0x46C0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD10, UINT16_C(0x46D0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD8, UINT16_C(0x32C0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD10, UINT16_C(0x32D0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD8, UINT16_C(0x44C0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U, 16U},
    {TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD10, UINT16_C(0x44D0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U, 16U},
    {TI_C54X_OPCODE_ID_ST_T_SMEM_MOD8, UINT16_C(0x8CC0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_T_REGISTER, 1U, 8U},
    {TI_C54X_OPCODE_ID_ST_T_SMEM_MOD10, UINT16_C(0x8CD0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_T_REGISTER, 1U, 8U},
};

static void
test_exact_rows_and_projection(void)
{
    unsigned expected = 0U;

    TI_C54X_TEST_CHECK(sizeof(rows) / sizeof(rows[0]) == 20U);
    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 215);
    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const circular_row *row = &rows[index];
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(row->id);
        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == row->id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == row->value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == row->mask);
        TI_C54X_TEST_CHECK(metadata->operand_layout == row->layout);
        TI_C54X_TEST_CHECK(metadata->input_resources == row->resources);
        TI_C54X_TEST_CHECK(metadata->program_word_count == row->words);
        expected += row->encodings;
    }
    TI_C54X_TEST_CHECK(expected == 240U);
}

static void
test_lookup_population_and_mask_identity(void)
{
    unsigned populated = 0U;
    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        if (id != TI_C54X_OPCODE_ID_UNKNOWN) {
            ++populated;
        }
        for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
            if (id == rows[index].id) {
                TI_C54X_TEST_CHECK((word & rows[index].mask) == rows[index].value);
            }
        }
    }
    TI_C54X_TEST_CHECK(populated == 17681U);
}

int
main(void)
{
    test_exact_rows_and_projection();
    test_lookup_population_and_mask_identity();
    return 0;
}
