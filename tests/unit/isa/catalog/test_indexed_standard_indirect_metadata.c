/*
 * Module contract
 *
 * Owns: executable metadata and generated-lookup proof for the twenty-two
 * private CMPT=0 AR0-indexed standard-indirect catalog rows. Does not own: decode
 * extraction, address arithmetic, semantic effects, CPU transactions, or
 * inventory quality policy.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../../src/isa/ops/dispatch/trusted_semantics.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

typedef struct indexed_row {
    ti_c54x_opcode_id id;
    uint16_t value;
    uint16_t mask;
    ti_c54x_operand_layout_id layout;
    ti_c54x_input_resource_mask resources;
    uint8_t words;
    unsigned encodings;
} indexed_row;

static const indexed_row rows[] = {
    {TI_C54X_OPCODE_ID_LD_SMEM_MOD5, UINT16_C(0x10A8), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U, 16U},
    {TI_C54X_OPCODE_ID_LD_SMEM_MOD6, UINT16_C(0x10B0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U, 16U},
    {TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD5, UINT16_C(0x14A8), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, 1U, 16U},
    {TI_C54X_OPCODE_ID_LD_SMEM_TS_MOD6, UINT16_C(0x14B0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER, 1U, 16U},
    {TI_C54X_OPCODE_ID_STL_SMEM_MOD5, UINT16_C(0x80A8), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS, 1U, 16U},
    {TI_C54X_OPCODE_ID_STL_SMEM_MOD6, UINT16_C(0x80B0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_ACCUMULATORS, 1U, 16U},
    {TI_C54X_OPCODE_ID_LDU_SMEM_MOD5, UINT16_C(0x12A8), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 1U, 16U},
    {TI_C54X_OPCODE_ID_LDU_SMEM_MOD6, UINT16_C(0x12B0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 1U, 16U},
    {TI_C54X_OPCODE_ID_LD_T_SMEM_MOD5, UINT16_C(0x30A8), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_LD_T_SMEM_MOD6, UINT16_C(0x30B0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD5, UINT16_C(0x76A8), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 2U, 8U},
    {TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD6, UINT16_C(0x76B0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,
     TI_C54X_INPUT_RESOURCES_NONE, 2U, 8U},
    {TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD5, UINT16_C(0x46A8), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_LD_DP_SMEM_MOD6, UINT16_C(0x46B0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD5, UINT16_C(0x32A8), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_LD_ASM_SMEM_MOD6, UINT16_C(0x32B0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_NONE, 1U, 8U},
    {TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD5, UINT16_C(0x44A8), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U, 16U},
    {TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD6, UINT16_C(0x44B0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U, 16U},
    {TI_C54X_OPCODE_ID_ST_T_SMEM_MOD5, UINT16_C(0x8CA8), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_T_REGISTER, 1U, 8U},
    {TI_C54X_OPCODE_ID_ST_T_SMEM_MOD6, UINT16_C(0x8CB0), UINT16_C(0xFFF8),
     TI_C54X_OPERAND_LAYOUT_ARF_MOD4, TI_C54X_INPUT_RESOURCES_T_REGISTER, 1U, 8U},
    {TI_C54X_OPCODE_ID_LDR_MOD5, UINT16_C(0x16A8), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U, 16U},
    {TI_C54X_OPCODE_ID_LDR_MOD6, UINT16_C(0x16B0), UINT16_C(0xFEF8),
     TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,
     TI_C54X_INPUT_RESOURCES_STATUS_SXM, 1U, 16U},
};

static const ti_c54x_trusted_op_handler indexed_handlers[] = {
    ti_c54x_trusted_op_ld_smem_mod5, ti_c54x_trusted_op_ld_smem_mod6,
    ti_c54x_trusted_op_ld_smem_ts_mod5, ti_c54x_trusted_op_ld_smem_ts_mod6,
    ti_c54x_trusted_op_stl_smem_mod5, ti_c54x_trusted_op_stl_smem_mod6,
    ti_c54x_trusted_op_ldu_smem_mod5, ti_c54x_trusted_op_ldu_smem_mod6,
    ti_c54x_trusted_op_ld_t_smem_mod5, ti_c54x_trusted_op_ld_t_smem_mod6,
    ti_c54x_trusted_op_st_immediate_smem_mod5,
    ti_c54x_trusted_op_st_immediate_smem_mod6,
    ti_c54x_trusted_op_ld_dp_smem_mod5, ti_c54x_trusted_op_ld_dp_smem_mod6,
    ti_c54x_trusted_op_ld_asm_smem_mod5,
    ti_c54x_trusted_op_ld_asm_smem_mod6,
    ti_c54x_trusted_op_ld_smem_shift16_mod5,
    ti_c54x_trusted_op_ld_smem_shift16_mod6,
    ti_c54x_trusted_op_st_t_smem_mod5, ti_c54x_trusted_op_st_t_smem_mod6,
    ti_c54x_trusted_op_ldr_mod5, ti_c54x_trusted_op_ldr_mod6,
};

static void
test_exact_rows_and_projection(void)
{
    unsigned expected = 0U;

    TI_C54X_TEST_CHECK(sizeof(rows) / sizeof(rows[0]) == 22U);
    TI_C54X_TEST_CHECK(sizeof(indexed_handlers) /
                           sizeof(indexed_handlers[0]) == 22U);
    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 261);
    for (size_t index = 0U; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        const indexed_row *row = &rows[index];
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(row->id);
        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == row->id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == row->value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == row->mask);
        TI_C54X_TEST_CHECK(metadata->operand_layout == row->layout);
        TI_C54X_TEST_CHECK(metadata->input_resources == row->resources);
        TI_C54X_TEST_CHECK(metadata->program_word_count == row->words);
        TI_C54X_TEST_CHECK(indexed_handlers[index] != NULL);
        expected += row->encodings;
    }
    TI_C54X_TEST_CHECK(expected == 272U);
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
    TI_C54X_TEST_CHECK(populated == 25617U);
}

int
main(void)
{
    test_exact_rows_and_projection();
    test_lookup_population_and_mask_identity();
    return 0;
}
