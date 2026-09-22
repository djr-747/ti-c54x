/*
 * Module contract
 *
 * Owns: executable proof that the MVDK_SMEM catalog row carries
 * the 0x7180 value under the 0xFF80 mask with two program words
 * (the indirect Smem cookie rides the low seven bits, the
 * 16-bit absolute dmad rides the second word), covering all
 * 128 first-word encodings with no shadow word.
 * Does not own: catalog generation, decode, address resolution,
 * MMR routing, CPU/state mutation, fetch or bus behavior,
 * sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static const uint16_t MVDK_SMEM_VALUE = UINT16_C(0x7180);
static const uint16_t MVDK_SMEM_MASK = UINT16_C(0xFF80);

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_MVDK_SMEM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 379);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_MVDK_SMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == MVDK_SMEM_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == MVDK_SMEM_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_INDIRECT_ADDR7_EXTENSION16);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x007F));
}

static void
test_exact_population(void)
{
    unsigned matches = 0U;
    uint32_t raw;

    for (raw = 0U; raw <= UINT32_C(0xFFFF); ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool matches_row =
            (uint16_t)(word & MVDK_SMEM_MASK) == MVDK_SMEM_VALUE;
        ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);

        if (matches_row) {
            TI_C54X_TEST_CHECK(id == TI_C54X_OPCODE_ID_MVDK_SMEM);
            matches++;
        } else if (id == TI_C54X_OPCODE_ID_MVDK_SMEM) {
            TI_C54X_TEST_CHECK(false);
        }
    }
    TI_C54X_TEST_CHECK(matches == 128U);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    return 0;
}
