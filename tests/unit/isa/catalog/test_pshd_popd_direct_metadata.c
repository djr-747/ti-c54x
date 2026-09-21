/*
 * Module contract
 *
 * Owns: executable proof that the PSHD/POPD/PSHM/POPM catalog
 * rows carry the 0x4B00/0x8B00/0x4A00/0x8A00 values each under
 * the 0xFF00 mask with one program word, the indirect-addr7
 * layout, stack-pointer input, and unresolved repeatability,
 * covering exactly the 256 first-word encodings per row with
 * no overlap into any neighboring row.
 * Does not own: catalog generation, decode, stack-pointer state,
 * CPU sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct pair_row {
    ti_c54x_opcode_id id;
    uint16_t value;
} pair_row;

static const pair_row ROWS[] = {
    {TI_C54X_OPCODE_ID_PSHD_DIRECT, UINT16_C(0x4B00)},
    {TI_C54X_OPCODE_ID_POPD_DIRECT, UINT16_C(0x8B00)},
    {TI_C54X_OPCODE_ID_PSHM_DIRECT, UINT16_C(0x4A00)},
    {TI_C54X_OPCODE_ID_POPM_DIRECT, UINT16_C(0x8A00)},
};

static void
test_metadata(void)
{
    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 353);

    for (size_t i = 0U; i < sizeof(ROWS) / sizeof(ROWS[0]); ++i) {
        const ti_c54x_opcode_metadata *metadata =
            ti_c54x_opcode_metadata_for_id(ROWS[i].id);

        TI_C54X_TEST_CHECK(metadata != NULL);
        TI_C54X_TEST_CHECK(metadata->id == ROWS[i].id);
        TI_C54X_TEST_CHECK(metadata->first_word_value == ROWS[i].value);
        TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFF00));
        TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
        TI_C54X_TEST_CHECK(metadata->operand_layout ==
                           TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7);
        TI_C54X_TEST_CHECK(metadata->repeatability ==
                           TI_C54X_REPEATABILITY_UNRESOLVED);
        TI_C54X_TEST_CHECK(metadata->input_resources ==
                           TI_C54X_INPUT_RESOURCES_STACK_POINTER);
        TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                           UINT16_C(0x00FF));
    }
}

static void
test_exact_population(void)
{
    for (size_t i = 0U; i < sizeof(ROWS) / sizeof(ROWS[0]); ++i) {
        unsigned matches = 0U;

        for (uint32_t raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
            const uint16_t word = (uint16_t)raw;
            const bool matches_row =
                (uint16_t)(word & UINT16_C(0xFF00)) == ROWS[i].value;
            ti_c54x_opcode_id id =
                ti_c54x_opcode_catalog_lookup_first_word(word);

            if (matches_row) {
                ++matches;
            }
            TI_C54X_TEST_CHECK((id == ROWS[i].id) == matches_row);
        }
        TI_C54X_TEST_CHECK(matches == 256U);
    }
}

static void
test_row_boundaries(void)
{
    /* PSHM 0x4A00 abuts unbound 0x49FF below and PSHD 0x4B00
     * above; PSHD abuts LTD_SMEM 0x4C00 above. POPM 0x8A00 abuts
     * unbound 0x89FF below and POPD 0x8B00 above; POPD abuts
     * ST_T_SMEM_DIRECT 0x8C00 above. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x49FF)) == TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x4A00)) ==
                       TI_C54X_OPCODE_ID_PSHM_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x4AFF)) ==
                       TI_C54X_OPCODE_ID_PSHM_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x4B00)) ==
                       TI_C54X_OPCODE_ID_PSHD_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x4BFF)) ==
                       TI_C54X_OPCODE_ID_PSHD_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x4C00)) == TI_C54X_OPCODE_ID_LTD_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x89FF)) == TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x8A00)) ==
                       TI_C54X_OPCODE_ID_POPM_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x8AFF)) ==
                       TI_C54X_OPCODE_ID_POPM_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x8B00)) ==
                       TI_C54X_OPCODE_ID_POPD_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x8BFF)) ==
                       TI_C54X_OPCODE_ID_POPD_DIRECT);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x8C00)) ==
                       TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
