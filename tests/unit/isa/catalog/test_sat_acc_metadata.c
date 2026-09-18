/*
 * Module contract
 *
 * Owns: executable proof that the SAT_ACC catalog row carries the
 * 0xF483 value under the 0xFEFF mask with one program word, the
 * shared accumulator/source layout, accumulator-only inputs, and
 * unresolved repeatability, covering both first-word encodings
 * with no shadow word.
 * Does not own: catalog generation, decode, accumulator/status
 * state, CPU sequencing, fetch or bus behavior, timing, or handler
 * bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t SAT_ACC_VALUE = UINT16_C(0xF483);
static const uint16_t SAT_ACC_MASK = UINT16_C(0xFEFF);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_SAT_ACC);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 304);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_SAT_ACC);
    TI_C54X_TEST_CHECK(metadata->first_word_value == SAT_ACC_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == SAT_ACC_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x0100));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & SAT_ACC_MASK) == SAT_ACC_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_SAT_ACC) == matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 2U);
}

static void
test_row_boundaries(void)
{
    /* The 0xF482/0xF582 neighbors belong to LD_SRC_ASM, not SAT_ACC. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF482)) == TI_C54X_OPCODE_ID_LD_SRC_ASM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF582)) == TI_C54X_OPCODE_ID_LD_SRC_ASM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF483)) == TI_C54X_OPCODE_ID_SAT_ACC);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xF583)) == TI_C54X_OPCODE_ID_SAT_ACC);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
