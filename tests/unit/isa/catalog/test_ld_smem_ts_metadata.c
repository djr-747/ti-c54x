/*
 * Module contract
 *
 * Owns: executable proof of the MOD2 LD Smem,TS,dst catalog row,
 * including its exact masked identity, D/ARF population, accumulator-ARF
 * layout, combined SXM/T input-resource metadata, and unresolved repeatability.
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

static const uint16_t LD_SMEM_TS_VALUE = UINT16_C(0x1480);
static const uint16_t LD_SMEM_TS_MOD2_WORD = UINT16_C(0x1490);
static const uint16_t LD_SMEM_TS_MASK = UINT16_C(0xFEE0);

static bool
matches_ld_smem_ts(uint16_t word)
{
    return (word & LD_SMEM_TS_MASK) == LD_SMEM_TS_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_LD_SMEM_TS);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 196);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_LD_SMEM_TS);
    TI_C54X_TEST_CHECK(metadata->first_word_value == LD_SMEM_TS_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == LD_SMEM_TS_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM_T_REGISTER);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           LD_SMEM_TS_MOD2_WORD) ==
                       TI_C54X_OPCODE_ID_LD_SMEM_TS);

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
        const bool matches = matches_ld_smem_ts(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_LD_SMEM_TS) ==
                           matches);
        if (matches) {
            ++encoded_count;
        }
    }

    TI_C54X_TEST_CHECK(encoded_count == 64U);
    for (uint16_t d = 0U; d < UINT16_C(2); ++d) {
        for (uint16_t arf = 0U; arf < UINT16_C(8); ++arf) {
            const uint16_t word = (uint16_t)(LD_SMEM_TS_VALUE |
                                             (uint16_t)(d << 8U) | arf);
            TI_C54X_TEST_CHECK(
                ti_c54x_opcode_catalog_lookup_first_word(word) ==
                TI_C54X_OPCODE_ID_LD_SMEM_TS);
        }
    }
}

static void
test_fixed_bits_and_near_misses_are_not_ts(void)
{
    /* Every bit other than D and ARF is fixed by the exact FEF8 mask. */
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x0020), UINT16_C(0x0040), UINT16_C(0x0080),
        UINT16_C(0x0200), UINT16_C(0x0400), UINT16_C(0x0800),
        UINT16_C(0x1000), UINT16_C(0x2000), UINT16_C(0x4000),
        UINT16_C(0x8000)};

    for (size_t index = 0U;
         index < sizeof(fixed_bit_masks) / sizeof(fixed_bit_masks[0]);
         ++index) {
        const uint16_t near_miss =
            (uint16_t)(LD_SMEM_TS_VALUE ^ fixed_bit_masks[index]);
        TI_C54X_TEST_CHECK(!matches_ld_smem_ts(near_miss));
        TI_C54X_TEST_CHECK(
            ti_c54x_opcode_catalog_lookup_first_word(near_miss) !=
            TI_C54X_OPCODE_ID_LD_SMEM_TS);
    }

    /* Bit 10 is the established LD Smem sibling, not an overaccepted TS row. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(LD_SMEM_TS_VALUE ^ UINT16_C(0x0400))) ==
                       TI_C54X_OPCODE_ID_LD_SMEM);
}

int
main(void)
{
    test_metadata();
    test_exact_population_and_disjointness();
    test_fixed_bits_and_near_misses_are_not_ts();
    return 0;
}
