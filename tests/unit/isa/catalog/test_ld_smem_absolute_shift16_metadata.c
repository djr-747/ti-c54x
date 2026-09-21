/*
 * Module contract
 *
 * Owns: executable proof that the
 * LD_SMEM_ABSOLUTE_SHIFT16 catalog row carries the
 * 0x44F8 exact value with two program words, the
 * destination+abs16 layout, SXM-only input resources,
 * and evidenced INELIGIBLE repeatability (absolute
 * operands cannot repeat), covering exactly the single
 * absolute first-word encoding with no shadow word and
 * no overlap into the shift16 MOD rows.
 * Does not own: catalog generation, decode, memory
 * addressing or buses, accumulator/T/status state, CPU
 * sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(
            TI_C54X_OPCODE_ID_LD_SMEM_ABSOLUTE_SHIFT16);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 356);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id ==
                       TI_C54X_OPCODE_ID_LD_SMEM_ABSOLUTE_SHIFT16);
    TI_C54X_TEST_CHECK(metadata->first_word_value == UINT16_C(0x44F8));
    TI_C54X_TEST_CHECK(metadata->first_word_mask == UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_INDIRECT_ADDR7_ABSOLUTE16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (word == UINT16_C(0x44F8)) {
            ++matches;
        }
        TI_C54X_TEST_CHECK((id ==
                            TI_C54X_OPCODE_ID_LD_SMEM_ABSOLUTE_SHIFT16) ==
                           (word == UINT16_C(0x44F8)));
    }
    TI_C54X_TEST_CHECK(matches == 1U);
}

static void
test_row_boundaries(void)
{
    /* 0x44F8 is the only absolute-shift16 encoding; the
     * MOD7 shift16 row keeps its words. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x44F8)) ==
                       TI_C54X_OPCODE_ID_LD_SMEM_ABSOLUTE_SHIFT16);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x44B8)) ==
                       TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16_MOD7);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_row_boundaries();
    return 0;
}
