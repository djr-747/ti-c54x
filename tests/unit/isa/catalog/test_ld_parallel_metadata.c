/* Module contract
 *
 * Owns: executable proof that the LD parallel-MAC/MAS catalog
 * rows carry the 0xA800/0xAC00 values under the 0xFC00 mask with
 * one program word, the ROUND_DESTINATION_DUALXY layout,
 * accumulator plus auxiliary-register plus live-T plus
 * SXM/FRCT/OVM inputs, and eligible repeatability, covering
 * all 1024 first-word encodings per row with UNKNOWN and live
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
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_PARALLEL_MAC);
    const ti_c54x_opcode_metadata *mas =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_PARALLEL_MAS);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 379);
    TI_C54X_TEST_CHECK(mac != NULL);
    TI_C54X_TEST_CHECK(mac->id == TI_C54X_OPCODE_ID_LD_PARALLEL_MAC);
    TI_C54X_TEST_CHECK(mac->first_word_value == UINT16_C(0xA800));
    TI_C54X_TEST_CHECK(mac->first_word_mask == UINT16_C(0xFC00));
    TI_C54X_TEST_CHECK(mac->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(mac->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ROUND_DESTINATION_DUALXY);
    TI_C54X_TEST_CHECK(mac->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(mac->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_T_REGISTER_STATUS_SXM_FRCT_OVM);
    TI_C54X_TEST_CHECK((uint16_t)~mac->first_word_mask ==
                       UINT16_C(0x03FF));
    TI_C54X_TEST_CHECK(mas != NULL);
    TI_C54X_TEST_CHECK(mas->id == TI_C54X_OPCODE_ID_LD_PARALLEL_MAS);
    TI_C54X_TEST_CHECK(mas->first_word_value == UINT16_C(0xAC00));
    TI_C54X_TEST_CHECK(mas->first_word_mask == UINT16_C(0xFC00));
    TI_C54X_TEST_CHECK(mas->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(mas->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ROUND_DESTINATION_DUALXY);
    TI_C54X_TEST_CHECK(mas->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(mas->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_T_REGISTER_STATUS_SXM_FRCT_OVM);
}

static unsigned
check_population(uint16_t value, ti_c54x_opcode_id expected_id)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & UINT16_C(0xFC00)) == value;
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
                           UINT16_C(0xA800),
                           TI_C54X_OPCODE_ID_LD_PARALLEL_MAC) == 1024U);
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xAC00),
                           TI_C54X_OPCODE_ID_LD_PARALLEL_MAS) == 1024U);
}

static void
test_row_boundaries(void)
{
    /* 0xA7FF is UNKNOWN; 0xB000 is the MAC-dual M0_M0 row. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xA7FF)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xA800)) ==
                       TI_C54X_OPCODE_ID_LD_PARALLEL_MAC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xABFF)) ==
                       TI_C54X_OPCODE_ID_LD_PARALLEL_MAC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xAC00)) ==
                       TI_C54X_OPCODE_ID_LD_PARALLEL_MAS);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xAFFF)) ==
                       TI_C54X_OPCODE_ID_LD_PARALLEL_MAS);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xB000)) ==
                       TI_C54X_OPCODE_ID_MAC_DUAL_M0_M0);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
