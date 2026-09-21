/*
 * Module contract
 *
 * Owns: executable proof of the MOD2 ST T-to-memory catalog row,
 * including its exact masked identity, ARF operand layout, repeatability and
 * T-register input-resource metadata, eight-word first-word population, and
 * fixed-bit separation. Does not own: opcode decoding, operand extraction,
 * address resolution, memory effects, CPU/state mutation, sequencing, timing,
 * generated lookup production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t ST_T_VALUE = UINT16_C(0x8C80);
static const uint16_t ST_T_MOD2_WORD = UINT16_C(0x8C90);
static const uint16_t ST_T_MASK = UINT16_C(0xFFE0);

static bool
matches_st_t(uint16_t word)
{
    return (word & ST_T_MASK) == ST_T_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_ST_T_SMEM);

    TI_C54X_TEST_CHECK(TI_C54X_OPCODE_ID_COUNT == 356);
    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id == TI_C54X_OPCODE_ID_ST_T_SMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == ST_T_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == ST_T_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(metadata->operand_layout == TI_C54X_OPERAND_LAYOUT_ARF_MOD);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_T_REGISTER);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           ST_T_MOD2_WORD) == TI_C54X_OPCODE_ID_ST_T_SMEM);

    /* Only ARF bits 2:0 vary; bit 8 and every other bit are fixed. */
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x001F));
}

static void
test_exact_population(void)
{
    unsigned st_t_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool st_t = matches_st_t(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_ST_T_SMEM) == st_t);
        if (st_t) {
            ++st_t_count;
        }
    }

    TI_C54X_TEST_CHECK(st_t_count == 32U);
    for (uint16_t arf = 0U; arf < UINT16_C(8); ++arf) {
        TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                               (uint16_t)(ST_T_VALUE | arf)) ==
                           TI_C54X_OPCODE_ID_ST_T_SMEM);
    }
}

static void
test_fixed_bit_near_misses_are_unknown(void)
{
    /* These flip fixed bits; the low three ARF bits remain. Clearing
     * bit 7 lands the pinned direct sibling (asserted below); bit 15
     * (0x8000) leaves in P3-S19: 0x8C90 ^ 0x8000 is 0x0C90, a valid
     * SUB_SMEM_TS word asserted below; setting bit 12 lands the
     * P3-S49 STRCD_XMEM_COND sibling (asserted below). */
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x0100), UINT16_C(0x0200), UINT16_C(0x0400),
        UINT16_C(0x0800), UINT16_C(0x2000),
        UINT16_C(0x4000)};

    for (size_t index = 0U;
         index < sizeof(fixed_bit_masks) / sizeof(fixed_bit_masks[0]);
         ++index) {
        const uint16_t word = (uint16_t)(ST_T_VALUE ^ fixed_bit_masks[index]);

        TI_C54X_TEST_CHECK(!matches_st_t(word));
        TI_C54X_TEST_CHECK(
            ti_c54x_opcode_catalog_lookup_first_word(word) ==
            TI_C54X_OPCODE_ID_UNKNOWN);
    }
    {
        const uint16_t direct_word = (uint16_t)(ST_T_VALUE ^ UINT16_C(0x0080));

        TI_C54X_TEST_CHECK(!matches_st_t(direct_word));
        TI_C54X_TEST_CHECK(
            ti_c54x_opcode_catalog_lookup_first_word(direct_word) ==
            TI_C54X_OPCODE_ID_ST_T_SMEM_DIRECT);
    }

    /* The nearby STL sibling remains distinct despite sharing ST's low bits. */
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           UINT16_C(0x8080)) ==
                       TI_C54X_OPCODE_ID_STL_SMEM);

    /* Setting bit 12 lands on 0x9C90, claimed by the P3-S49
     * STRCD_XMEM_COND row: a valid cross-row near miss, not an
     * overaccept. */
    TI_C54X_TEST_CHECK(!matches_st_t(
        (uint16_t)(ST_T_VALUE ^ UINT16_C(0x1000))));
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(ST_T_VALUE ^ UINT16_C(0x1000))) ==
                       TI_C54X_OPCODE_ID_STRCD_XMEM_COND);

    /* Flipping bit 15 lands on 0x0C90, claimed by the P3-S19
     * SUB_SMEM_TS row: a valid cross-row near miss, not an
     * overaccept. */
    TI_C54X_TEST_CHECK(!matches_st_t(
        (uint16_t)(ST_T_VALUE ^ UINT16_C(0x8000))));
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(ST_T_VALUE ^ UINT16_C(0x8000))) ==
                       TI_C54X_OPCODE_ID_SUB_SMEM_TS);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_fixed_bit_near_misses_are_unknown();
    return 0;
}
