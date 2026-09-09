/*
 * Module contract
 *
 * Owns: executable proof of the MOD2 LD Smem,16,dst catalog row,
 * including its exact masked identity, D/ARF population, accumulator layout,
 * SXM input-resource and repeatability metadata, and fixed-bit separation.
 * Does not own: opcode decoding, operand extraction, address resolution,
 * memory effects, CPU/state mutation, sequencing, timing, generated lookup
 * production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t LD_SMEM_SHIFT16_VALUE = UINT16_C(0x4480);
static const uint16_t LD_SMEM_SHIFT16_MOD2_WORD = UINT16_C(0x4490);
static const uint16_t LD_SMEM_SHIFT16_MASK = UINT16_C(0xFEE0);

static bool
matches_ld_smem_shift16(uint16_t word)
{
    return (word & LD_SMEM_SHIFT16_MASK) == LD_SMEM_SHIFT16_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(
            TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 139);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id ==
                       TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16);
    TI_C54X_TEST_CHECK(metadata->first_word_value ==
                       LD_SMEM_SHIFT16_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == LD_SMEM_SHIFT16_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LD_SMEM_SHIFT16_MOD2_WORD) ==
                       TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16);

    /* D is bit 8 and ARF is bits 2:0; all other bits are fixed. */
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x011F));
}

static void
test_exact_population_and_disjointness(void)
{
    unsigned encoded_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool matches = matches_ld_smem_shift16(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16) ==
                           matches);
        if (matches) {
            ++encoded_count;
        }
    }

    TI_C54X_TEST_CHECK(encoded_count == 64U);
    for (uint16_t d = 0U; d < UINT16_C(2); ++d) {
        for (uint16_t arf = 0U; arf < UINT16_C(8); ++arf) {
            const uint16_t word = (uint16_t)(LD_SMEM_SHIFT16_VALUE |
                                             (uint16_t)(d << 8U) | arf);
            TI_C54X_TEST_CHECK(
                ti_c54x_opcode_catalog_lookup_first_word(word) ==
                TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16);
        }
    }
}

static void
test_fixed_bits_and_lax_mask_overaccept_are_rejected(void)
{
    /* Every bit other than D and ARF is fixed by the exact FE F8 mask. */
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x0080),
        UINT16_C(0x0400), UINT16_C(0x0800),
        UINT16_C(0x1000), UINT16_C(0x2000), UINT16_C(0x4000),
        UINT16_C(0x8000)};

    for (size_t index = 0U;
         index < sizeof(fixed_bit_masks) / sizeof(fixed_bit_masks[0]);
         ++index) {
        const uint16_t word =
            (uint16_t)(LD_SMEM_SHIFT16_VALUE ^ fixed_bit_masks[index]);
        TI_C54X_TEST_CHECK(!matches_ld_smem_shift16(word));
        TI_C54X_TEST_CHECK(
            ti_c54x_opcode_catalog_lookup_first_word(word) ==
            TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* Bit 9 alone reaches the intentional W37 DP sibling; adding fixed bit
     * 3 must not let that sibling pattern overaccept the W38 near miss. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(LD_SMEM_SHIFT16_VALUE ^
                                      UINT16_C(0x0200))) ==
                       TI_C54X_OPCODE_ID_LD_DP_SMEM);
    /* Mode bits are now part of the neutral row; MOD2 remains a regression
     * vector while adjacent mode encodings classify to the same row. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(LD_SMEM_SHIFT16_VALUE ^
                                      UINT16_C(0x0018))) ==
                       TI_C54X_OPCODE_ID_LD_SMEM_SHIFT16);
}

int
main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    test_fixed_bits_and_lax_mask_overaccept_are_rejected();
    return 0;
}
