/*
 * Module contract
 *
 * Owns: executable proof of the four STH syntax-4 MOD0-MOD3 catalog rows,
 * including their exact masked identities (0x6F80/0x6F88/0x6F90/0x6F98
 * under FFF8, MOD in first-word bits 6:3, ARF in bits 2:0), the shared
 * ACCUMULATOR_ARF_MOD4_SHIFT5 layout with
 * ACCUMULATORS_AUXILIARY_REGISTERS resources, first-word lookup staying
 * LD_SMEM_SHIFT by declaration order, and word-2 qualification (0x0C60
 * under FEE0) resolving each row while LD words keep the LD row.
 * Does not own: opcode decoding, operand extraction, address resolution,
 * memory effects, CPU/state mutation, sequencing, timing, generated
 * lookup production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct mod_row {
    ti_c54x_opcode_id id;
    uint16_t value;
    uint8_t mode;
} mod_row;

static const mod_row ROWS[] = {
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD0, UINT16_C(0x6F80), UINT8_C(0)},
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD1, UINT16_C(0x6F88), UINT8_C(1)},
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD2, UINT16_C(0x6F90), UINT8_C(2)},
    {TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD3, UINT16_C(0x6F98), UINT8_C(3)},
};

static const uint16_t ROW_MASK = UINT16_C(0xFFF8);
static const uint16_t LD_W2 = UINT16_C(0x0C40);
static const uint16_t STH_W2 = UINT16_C(0x0C60);

static void
test_metadata(void)
{
    for (size_t index = 0U; index < sizeof(ROWS) / sizeof(ROWS[0]);
         ++index) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(ROWS[index].id);

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == ROWS[index].id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == ROWS[index].value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == ROW_MASK);
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
        TI_C54X_TEST_CHECK(
            metadata->operand_layout ==
            TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4_SHIFT5);
        TI_C54X_TEST_CHECK(metadata->repeatability ==
                           TI_C54X_REPEATABILITY_UNRESOLVED);
        TI_C54X_TEST_CHECK(
            metadata->input_resources ==
            TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS);
        /* MOD lives in bits 6:3; ARF lives in bits 2:0. */
        TI_C54X_TEST_CHECK(
            (uint16_t)((ROWS[index].value >> 3U) & UINT16_C(0x000F)) ==
            ROWS[index].mode);
    }
    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 375);
}

static void
test_first_word_lookup_stays_ld(void)
{
    /* Declaration order keeps the LD row first, so every covered first
     * word still classifies LD; the slot table re-resolves on word 2. */
    for (size_t index = 0U; index < sizeof(ROWS) / sizeof(ROWS[0]);
         ++index) {
        for (uint16_t arf = UINT16_C(0); arf <= UINT16_C(7); ++arf) {
            const uint16_t word =
                (uint16_t)(ROWS[index].value | arf);

            TI_C54X_TEST_CHECK(
                ti_c54x_opcode_catalog_lookup_first_word(word) ==
                TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
            TI_C54X_TEST_CHECK(
                ti_c54x_resolve_qualified_slot(word, STH_W2) ==
                ROWS[index].id);
            TI_C54X_TEST_CHECK(
                ti_c54x_resolve_qualified_slot(word, LD_W2) ==
                TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
        }
    }
}

int
main(void)
{
    test_metadata();
    test_first_word_lookup_stays_ld();
    return 0;
}
