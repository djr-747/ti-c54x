/* Module contract
 *
 * Owns: executable proof that the ST parallel-MAC/MAS/MPY
 * catalog rows carry the 0xD000/0xD800 values under the 0xF800
 * mask and 0xCC00 under 0xFC00 with one program word, the
 * ROUND_SRC_DEST_DUALXY layout for MAC/MAS (SRC_DEST_DUALXY
 * for MPY), accumulator plus auxiliary-register plus
 * ASM/T/FRCT/OVM inputs, and eligible repeatability, covering
 * the full first-word population per row with UNKNOWN/sibling
 * neighbors.
 * Does not own: catalog generation, decode, memory addressing or
 * buses, status state, CPU sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *mac =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ST_PARALLEL_MAC);
    const ti_c54x_opcode_metadata *mas =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ST_PARALLEL_MAS);
    const ti_c54x_opcode_metadata *mpy =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ST_PARALLEL_MPY);

    TI_C54X_TEST_CHECK(mac != NULL);
    TI_C54X_TEST_CHECK(mac->id == TI_C54X_OPCODE_ID_ST_PARALLEL_MAC);
    TI_C54X_TEST_CHECK(mac->first_word_value == UINT16_C(0xD000));
    TI_C54X_TEST_CHECK(mac->first_word_mask == UINT16_C(0xF800));
    TI_C54X_TEST_CHECK(mac->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(mac->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ROUND_SRC_DEST_DUALXY);
    TI_C54X_TEST_CHECK(mac->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(mac->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_T_REGISTER_STATUS_FRCT_OVM);
    TI_C54X_TEST_CHECK((uint16_t)~mac->first_word_mask ==
                       UINT16_C(0x07FF));
    TI_C54X_TEST_CHECK(mas != NULL);
    TI_C54X_TEST_CHECK(mas->id == TI_C54X_OPCODE_ID_ST_PARALLEL_MAS);
    TI_C54X_TEST_CHECK(mas->first_word_value == UINT16_C(0xD800));
    TI_C54X_TEST_CHECK(mas->first_word_mask == UINT16_C(0xF800));
    TI_C54X_TEST_CHECK(mas->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(mas->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ROUND_SRC_DEST_DUALXY);
    TI_C54X_TEST_CHECK(mas->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(mas->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_T_REGISTER_STATUS_FRCT_OVM);
    TI_C54X_TEST_CHECK(mpy != NULL);
    TI_C54X_TEST_CHECK(mpy->id == TI_C54X_OPCODE_ID_ST_PARALLEL_MPY);
    TI_C54X_TEST_CHECK(mpy->first_word_value == UINT16_C(0xCC00));
    TI_C54X_TEST_CHECK(mpy->first_word_mask == UINT16_C(0xFC00));
    TI_C54X_TEST_CHECK(mpy->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(mpy->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SRC_DEST_DUALXY);
    TI_C54X_TEST_CHECK(mpy->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(mpy->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_T_REGISTER_STATUS_FRCT_OVM);
    TI_C54X_TEST_CHECK((uint16_t)~mpy->first_word_mask ==
                       UINT16_C(0x03FF));
}

static unsigned
check_population(uint16_t value, uint16_t mask, ti_c54x_opcode_id expected_id)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row = (uint16_t)(word & mask) == value;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            TI_C54X_TEST_CHECK(id == expected_id);
            matches++;
        } else if (id == expected_id) {
            TI_C54X_TEST_CHECK(false);
        }
    }
    return matches;
}

static void
test_exact_population(void)
{
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xD000), UINT16_C(0xF800),
                           TI_C54X_OPCODE_ID_ST_PARALLEL_MAC) == 2048U);
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xD800), UINT16_C(0xF800),
                           TI_C54X_OPCODE_ID_ST_PARALLEL_MAS) == 2048U);
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xCC00), UINT16_C(0xFC00),
                           TI_C54X_OPCODE_ID_ST_PARALLEL_MPY) == 1024U);
}

static void
test_row_boundaries(void)
{
    /* 0xCBFF is UNKNOWN; the MPY row tiles 0xCC00 through 0xCFFF,
     * the MAC row 0xD000 through 0xD7FF, the MAS row 0xD800
     * through 0xDFFF; 0xE000 is FIRS. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xCBFF)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xCC00)) ==
                       TI_C54X_OPCODE_ID_ST_PARALLEL_MPY);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xCFFF)) ==
                       TI_C54X_OPCODE_ID_ST_PARALLEL_MPY);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xD000)) ==
                       TI_C54X_OPCODE_ID_ST_PARALLEL_MAC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xD7FF)) ==
                       TI_C54X_OPCODE_ID_ST_PARALLEL_MAC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xD800)) ==
                       TI_C54X_OPCODE_ID_ST_PARALLEL_MAS);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xDFFF)) ==
                       TI_C54X_OPCODE_ID_ST_PARALLEL_MAS);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
