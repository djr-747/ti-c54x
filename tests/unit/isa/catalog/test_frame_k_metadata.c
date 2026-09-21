/*
 * Module contract
 *
 * Owns: executable proof that the FRAME catalog row carries the
 * 0xEE00 value under the 0xFF00 mask with one program word, the
 * FRAME_K8 layout, stack-pointer input, and unresolved
 * repeatability, covering exactly the 256 first-word encodings
 * with no shadow word and no overlap into any neighboring row.
 * Does not own: catalog generation, decode, stack-pointer state,
 * CPU sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t FRAME_VALUE = UINT16_C(0xEE00);
static const uint16_t FRAME_MASK = UINT16_C(0xFF00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_FRAME);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 371);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_FRAME);
    TI_C54X_TEST_CHECK(metadata->first_word_value == FRAME_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == FRAME_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_FRAME_K8);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x00FF));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & FRAME_MASK) == FRAME_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_FRAME) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 256U);
}

static void
test_row_boundaries(void)
{
    /* 0xEE00 and 0xEEFF are the slice ends; RPT_K ends at 0xECFF
     * and LD_IMMEDIATE_ASM at 0xED1F, so both neighbors here are
     * unbound. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xEE00)) == TI_C54X_OPCODE_ID_FRAME);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xEE10)) == TI_C54X_OPCODE_ID_FRAME);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xEEFF)) == TI_C54X_OPCODE_ID_FRAME);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xEDFF)) == TI_C54X_OPCODE_ID_UNKNOWN);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0xEF00)) == TI_C54X_OPCODE_ID_UNKNOWN);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
