/*
 * Module contract
 *
 * Owns: executable proof of the two-word MOD2 ST immediate-to-memory
 * catalog row, including its exact masked identity, ARF-plus-extension
 * operand layout, repeatability and input-resource metadata, eight-word
 * first-word population, and fixed-bit separation. Does not own: opcode
 * decoding, operand extraction, address resolution, memory effects, CPU/state
 * mutation, sequencing, timing, generated lookup production, or handler
 * bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t ST_IMMEDIATE_VALUE = UINT16_C(0x7680);
static const uint16_t ST_IMMEDIATE_MOD2_WORD = UINT16_C(0x7690);
static const uint16_t ST_IMMEDIATE_MASK = UINT16_C(0xFFE0);

static bool
matches_st_immediate(uint16_t word)
{
    return (word & ST_IMMEDIATE_MASK) == ST_IMMEDIATE_VALUE;
}

static void
test_metadata(void)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(
            TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(metadata->id ==
                       TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
    TI_C54X_TEST_CHECK(metadata->first_word_value == ST_IMMEDIATE_VALUE);
    TI_C54X_TEST_CHECK(metadata->first_word_mask == ST_IMMEDIATE_MASK);
    TI_C54X_TEST_CHECK(metadata->program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(metadata->operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD);
    TI_C54X_TEST_CHECK(metadata->repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(metadata->input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           ST_IMMEDIATE_MOD2_WORD) ==
                       TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);

    /* Only ARF bits 2:0 vary; bit 8 and every other bit are fixed. */
    TI_C54X_TEST_CHECK((uint16_t)~metadata->first_word_mask ==
                       UINT16_C(0x001F));
}

static void
test_exact_population(void)
{
    unsigned st_immediate_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id id =
            ti_c54x_opcode_catalog_lookup_first_word(word);
        const bool st_immediate = matches_st_immediate(word);

        TI_C54X_TEST_CHECK((id == TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM) ==
                           st_immediate);
        if (st_immediate) {
            ++st_immediate_count;
        }
    }

    TI_C54X_TEST_CHECK(st_immediate_count == 32U);
    for (uint16_t arf = 0U; arf < UINT16_C(8); ++arf) {
        TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                               (uint16_t)(ST_IMMEDIATE_VALUE | arf)) ==
                           TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM);
    }
}

static void
test_fixed_bit_near_misses_are_unknown(void)
{
    /* These flip fixed bits, except bit 8, which lands in the STM
     * row (0x7780), bit 9, which lands in the P3-S6 PORTR
     * indirect row (0x74A0, TI SPRU172C 4-129 pins PORTR at 0x74xx),
     * bit 11, which lands in the P3-S64 READA Smem row (0x7E80,
     * handled below), bit 12, which lands in the P3-S33
     * MAC_SMEM_LK row (0x6690, handled below), and bit 14, which
     * lands in the P3-S111 POLY row (0x3690 with I set); the low
     * three ARF bits remain. */
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x0400),
        UINT16_C(0x2000)};

    for (size_t index = 0U;
         index < sizeof(fixed_bit_masks) / sizeof(fixed_bit_masks[0]);
         ++index) {
        const uint16_t word =
            (uint16_t)(ST_IMMEDIATE_VALUE ^ fixed_bit_masks[index]);

        TI_C54X_TEST_CHECK(!matches_st_immediate(word));
        TI_C54X_TEST_CHECK(
            ti_c54x_opcode_catalog_lookup_first_word(word) ==
            TI_C54X_OPCODE_ID_UNKNOWN);
    }

    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(ST_IMMEDIATE_VALUE ^
                                      UINT16_C(0x0100))) ==
                       TI_C54X_OPCODE_ID_STM);

    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(ST_IMMEDIATE_VALUE ^
                                      UINT16_C(0x0200))) ==
                       TI_C54X_OPCODE_ID_PORTR_SMEM);

    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(ST_IMMEDIATE_VALUE ^ UINT16_C(0x0040))) ==
                       TI_C54X_OPCODE_ID_ST_IMMEDIATE_SMEM_MOD8);

    /* Flipping bit 7 lands on 0x7600, claimed by the P3-S36
     * ST_LK_SMEM row (direct dmad 0): a valid cross-row near
     * miss, not an overaccept. */
    TI_C54X_TEST_CHECK(!matches_st_immediate(
        (uint16_t)(ST_IMMEDIATE_VALUE ^ UINT16_C(0x0080))));
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(ST_IMMEDIATE_VALUE ^
                                      UINT16_C(0x0080))) ==
                       TI_C54X_OPCODE_ID_ST_LK_SMEM);

    /* Flipping bit 11 lands on 0x7E80, claimed by the P3-S64
     * READA_SMEM row: a valid cross-row near miss, not an
     * overaccept. */
    TI_C54X_TEST_CHECK(!matches_st_immediate(
        (uint16_t)(ST_IMMEDIATE_VALUE ^ UINT16_C(0x0800))));
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(ST_IMMEDIATE_VALUE ^
                                      UINT16_C(0x0800))) ==
                       TI_C54X_OPCODE_ID_READA_SMEM);

    /* Flipping bit 12 lands on 0x6690, claimed by the P3-S33
     * MAC_SMEM_LK row: a valid cross-row near miss, not an
     * overaccept. */
    TI_C54X_TEST_CHECK(!matches_st_immediate(
        (uint16_t)(ST_IMMEDIATE_VALUE ^ UINT16_C(0x1000))));
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(ST_IMMEDIATE_VALUE ^
                                      UINT16_C(0x1000))) ==
                       TI_C54X_OPCODE_ID_MAC_SMEM_LK);

    /* Flipping bit 15 lands on 0xF680, claimed by the P3-S29
     * ADD_SRC_ASM row (the 0xFCFF mask clears bits 9:8): a valid
     * cross-row near miss, not an overaccept. */
    TI_C54X_TEST_CHECK(!matches_st_immediate(
        (uint16_t)(ST_IMMEDIATE_VALUE ^ UINT16_C(0x8000))));
    TI_C54X_TEST_CHECK(ti_c54x_opcode_catalog_lookup_first_word(
                           (uint16_t)(ST_IMMEDIATE_VALUE ^
                                      UINT16_C(0x8000))) ==
                       TI_C54X_OPCODE_ID_ADD_SRC_ASM);
}

int
main(void)
{
    test_metadata();
    test_exact_population();
    test_fixed_bit_near_misses_are_unknown();
    return 0;
}
