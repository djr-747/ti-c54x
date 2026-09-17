/*
 * Module contract
 *
 * Owns: executable proof of the LD src[,SHIFT],dst catalog row
 * (syntax 10), including its exact masked identity, S/D/SHIFT5
 * population, reused accumulator layout, SXM/OVM input resources and
 * repeatability metadata, and fixed-bit separation.
 * Does not own: opcode decoding, operand extraction, accumulator reads,
 * CPU/state mutation, sequencing, timing, generated lookup production,
 * or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t LD_SRC_SHIFT_VALUE = UINT16_C(0xF440);
static const uint16_t LD_SRC_SHIFT_SAMPLE_WORD = UINT16_C(0xF75E);
static const uint16_t LD_SRC_SHIFT_MASK = UINT16_C(0xFCE0);

static bool
matches_ld_src_shift(uint16_t word)
{
    return (word & LD_SRC_SHIFT_MASK) == LD_SRC_SHIFT_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_SRC_SHIFT);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 262);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LD_SRC_SHIFT);
    TI_C54X_TEST_CHECK(metadata->first_word_value == LD_SRC_SHIFT_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == LD_SRC_SHIFT_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LD_SRC_SHIFT_SAMPLE_WORD) ==
                       TI_C54X_OPCODE_ID_LD_SRC_SHIFT);

    /* Source is bit 9, destination is bit 8, SHIFT fills bits 4:0; all
     * other first-word bits are fixed. */
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x031F));
}

static void
test_exact_population_and_disjointness(void)
{
    unsigned encoded_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool matches = matches_ld_src_shift(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_LD_SRC_SHIFT) ==
                           matches);
        if (matches) {
            ++encoded_count;
        }
    }

    TI_C54X_TEST_CHECK(encoded_count == 128U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LD_SRC_SHIFT_VALUE) ==
                       TI_C54X_OPCODE_ID_LD_SRC_SHIFT);
}

int main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    return 0;
}
