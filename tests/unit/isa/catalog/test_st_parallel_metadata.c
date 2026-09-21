/* Module contract
 *
 * Owns: executable proof that the ST parallel-ADD/SUB catalog
 * rows carry the 0xC000/0xC400 values under the 0xFC00 mask with
 * one program word, the SRC_DEST_DUALXY layout,
 * accumulator plus auxiliary-register plus ASM/SXM/OVM inputs,
 * and eligible repeatability, covering all 1024 first-word
 * encodings per row with UNKNOWN neighbors.
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
    const ti_c54x_opcode_metadata *add =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ST_PARALLEL_ADD);
    const ti_c54x_opcode_metadata *sub =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ST_PARALLEL_SUB);

    TI_C54X_TEST_CHECK(add != NULL);
    TI_C54X_TEST_CHECK(add->id == TI_C54X_OPCODE_ID_ST_PARALLEL_ADD);
    TI_C54X_TEST_CHECK(add->first_word_value == UINT16_C(0xC000));
    TI_C54X_TEST_CHECK(add->first_word_mask == UINT16_C(0xFC00));
    TI_C54X_TEST_CHECK(add->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(add->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SRC_DEST_DUALXY);
    TI_C54X_TEST_CHECK(add->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(add->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_STATUS_SXM_OVM);
    TI_C54X_TEST_CHECK((uint16_t)~add->first_word_mask ==
                       UINT16_C(0x03FF));
    TI_C54X_TEST_CHECK(sub != NULL);
    TI_C54X_TEST_CHECK(sub->id == TI_C54X_OPCODE_ID_ST_PARALLEL_SUB);
    TI_C54X_TEST_CHECK(sub->first_word_value == UINT16_C(0xC400));
    TI_C54X_TEST_CHECK(sub->first_word_mask == UINT16_C(0xFC00));
    TI_C54X_TEST_CHECK(sub->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(sub->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_SRC_DEST_DUALXY);
    TI_C54X_TEST_CHECK(sub->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(sub->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_AUXILIARY_REGISTERS_ASM_STATUS_SXM_OVM);
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
                           UINT16_C(0xC000),
                           TI_C54X_OPCODE_ID_ST_PARALLEL_ADD) == 1024U);
    TI_C54X_TEST_CHECK(check_population(
                           UINT16_C(0xC400),
                           TI_C54X_OPCODE_ID_ST_PARALLEL_SUB) == 1024U);
}

static void
test_row_boundaries(void)
{
    /* 0xBFFF and 0xC800 are UNKNOWN; the rows tile 0xC000
     * through 0xC7FF with no gap. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xBFFF)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xC000)) ==
                       TI_C54X_OPCODE_ID_ST_PARALLEL_ADD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xC3FF)) ==
                       TI_C54X_OPCODE_ID_ST_PARALLEL_ADD);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xC400)) ==
                       TI_C54X_OPCODE_ID_ST_PARALLEL_SUB);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xC7FF)) ==
                       TI_C54X_OPCODE_ID_ST_PARALLEL_SUB);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xC800)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
