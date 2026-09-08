/*
 * Module contract
 *
 * Owns: executable proof of the LDU *ARx+,dst MOD2 catalog row,
 * including its exact masked identity, accumulator/ARF layout,
 * repeatability/resource metadata, sixteen-word population, and separation
 * from the existing signed LD row. Does not own: opcode decoding, operand
 * extraction, memory effects, address resolution, CPU/state mutation,
 * sequencing, timing, generated lookup production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t LDU_VALUE = UINT16_C(0x1280);
static const uint16_t LDU_MOD2_WORD = UINT16_C(0x1290);
static const uint16_t LDU_MASK = UINT16_C(0xFEE0);

static bool
matches_ldu(uint16_t word)
{
    return (word & LDU_MASK) == LDU_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LDU_SMEM);
    const ti_c54x_opcode_metadata *ld_metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_SMEM);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LDU_SMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == LDU_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == LDU_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x011F));
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LDU_MOD2_WORD) == TI_C54X_OPCODE_ID_LDU_SMEM);

    /* LDU differs from the existing LD row at fixed bit 9, not by aliasing. */
    TI_C54X_TEST_CHECK(ld_metadata != NULL);
    TI_C54X_TEST_CHECK(ld_metadata->id != metadata->id);
    TI_C54X_TEST_CHECK(ld_metadata->first_word_value == UINT16_C(0x1080));
    TI_C54X_TEST_CHECK((LDU_VALUE & LDU_MASK) !=
                       (ld_metadata->first_word_value & LDU_MASK));
}

static void
test_population_and_lookup(void)
{
    unsigned ldu_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool ldu = matches_ldu(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_LDU_SMEM) == ldu);
        if (ldu) {
            ++ldu_count;
        }
    }

    /* D bit 8 and ARF bits 2:0 are the only variable first-word fields. */
    TI_C54X_TEST_CHECK(ldu_count == 64U);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(LDU_VALUE) ==
                       TI_C54X_OPCODE_ID_LDU_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(LDU_VALUE | UINT16_C(0x0107))) ==
                       TI_C54X_OPCODE_ID_LDU_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x1080)) ==
                       TI_C54X_OPCODE_ID_LD_SMEM);
}

static void
test_fixed_bit_near_misses(void)
{
    static const uint16_t unknown_near_misses[] = {
        UINT16_C(0x0020), UINT16_C(0x0040), UINT16_C(0x0080),
        UINT16_C(0x0800),
        UINT16_C(0x1000), UINT16_C(0x4000),
        UINT16_C(0x8000)};

    for (size_t index = 0U;
         index < sizeof(unknown_near_misses) /
                    sizeof(unknown_near_misses[0]);
         ++index) {
        const uint16_t word =
            (uint16_t)(LDU_VALUE ^ unknown_near_misses[index]);
        TI_C54X_TEST_CHECK(!matches_ldu(word));
        TI_C54X_TEST_CHECK(
            ti_c54x_opcode_catalog_lookup_first_word(word) ==
            TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* Flipping fixed bit 9 reaches the intentional LD sibling, never LDU. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(LDU_VALUE ^ UINT16_C(0x0200))) ==
                       TI_C54X_OPCODE_ID_LD_SMEM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(LDU_VALUE ^ UINT16_C(0x2000))) ==
                       TI_C54X_OPCODE_ID_LD_ASM_SMEM);
}

int
main(void)
{
    test_metadata();
    test_population_and_lookup();
    test_fixed_bit_near_misses();
    return 0;
}
