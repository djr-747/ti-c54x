/*
 * Module contract
 *
 * Owns: executable proof that the MPYU_SMEM catalog row carries the
 * 0x2400 value under the 0xFE00 mask with one program word, the
 * shared destination/indirect/addr7 layout, accumulator/T/FRCT/OVM
 * inputs, and eligible repeatability, covering all 512 first-word
 * encodings with no shadow word.
 * Does not own: catalog generation, decode, accumulator/T/status
 * state, CPU sequencing, fetch or bus behavior, timing, or handler
 * bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t MPYU_SMEM_VALUE = UINT16_C(0x2400);
static const uint16_t MPYU_SMEM_MASK = UINT16_C(0xFE00);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MPYU_SMEM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 325);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_MPYU_SMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == MPYU_SMEM_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == MPYU_SMEM_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DESTINATION_INDIRECT_ADDR7);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_ELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_T_REGISTER_STATUS_FRCT_OVM);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x01FF));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & MPYU_SMEM_MASK) == MPYU_SMEM_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_MPYU_SMEM) ==
                           matches_row);
    }
    TI_C54X_TEST_CHECK(matches == 512U);
}

static void
test_row_boundaries(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x23FF)) != TI_C54X_OPCODE_ID_MPYU_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x2600)) != TI_C54X_OPCODE_ID_MPYU_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x2400)) == TI_C54X_OPCODE_ID_MPYU_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x25FF)) == TI_C54X_OPCODE_ID_MPYU_SMEM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
