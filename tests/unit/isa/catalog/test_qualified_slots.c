/*
 * Module contract
 *
 * Owns: executable proof that second-word slot resolution qualifies the
 * 0x6F80/FFE0 LD Smem,SHIFT slot on the second program word under mask
 * 0xFEE0: 0x0C40 words resolve LD_SMEM_SHIFT, 0x0C60 words resolve the
 * STH_SHIFT_SMEM_MOD0-MOD3 row selected by first-word bits 6:3,
 * 0x0C00 words resolve ADD_SMEM_SHIFT and 0x0C20 words resolve
 * SUB_SMEM_SHIFT, and every other second word resolves UNKNOWN. The
 * bit-5 parity between the LD and STH fixed patterns is the whole
 * discriminator; the syn4 patterns sit outside the 0xFEE0 mask.
 * Does not own: lookup generation, decode policy, dispatch, CPU/state
 * mutation, sequencing, timing, or diagnostics.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/catalog/opcode_catalog_internal.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t SLOT_FIRST = UINT16_C(0x6F80);
static const uint16_t LD_W2 = UINT16_C(0x0C40);
static const uint16_t STH_W2 = UINT16_C(0x0C60);

static uint16_t
mod_first_word(uint8_t mode, uint8_t arf)
{
    return (uint16_t)(SLOT_FIRST |
                      (uint16_t)(((uint16_t)(mode & UINT8_C(3))) << 3U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static void
test_slot_claimed(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_first_word_has_qualified_slot(SLOT_FIRST) ==
                       true);
    TI_C54X_TEST_CHECK(ti_c54x_first_word_has_qualified_slot(
                           mod_first_word(UINT8_C(2), UINT8_C(5))) == true);
    TI_C54X_TEST_CHECK(ti_c54x_first_word_has_qualified_slot(
                           UINT16_C(0x0000)) == false);
}

static void
test_ld_word_resolves_ld(void)
{
    for (uint8_t mode = UINT8_C(0); mode <= UINT8_C(3); ++mode) {
        for (uint8_t arf = UINT8_C(0); arf <= UINT8_C(7); ++arf) {
            TI_C54X_TEST_CHECK(
                ti_c54x_resolve_qualified_slot(
                    mod_first_word(mode, arf),
                    (uint16_t)(LD_W2 | (uint16_t)(arf & UINT8_C(7)))) ==
                TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
        }
    }
}

static void
test_sth_word_resolves_mod_row(void)
{
    static const ti_c54x_opcode_id expected[4] = {
        TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD0,
        TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD1,
        TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD2,
        TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD3};
    for (uint8_t mode = UINT8_C(0); mode <= UINT8_C(3); ++mode) {
        for (uint8_t arf = UINT8_C(0); arf <= UINT8_C(7); ++arf) {
            TI_C54X_TEST_CHECK(
                ti_c54x_resolve_qualified_slot(
                    mod_first_word(mode, arf),
                    (uint16_t)(STH_W2 | (uint16_t)(arf & UINT8_C(7)))) ==
                expected[mode]);
        }
    }
}

static void
test_parity_bit_selects_family(void)
{
    /* 0x0C40 and 0x0C60 differ only in bit 5; flipping it flips the
     * resolved family for every first word in the slot. */
    TI_C54X_TEST_CHECK((uint16_t)(LD_W2 ^ STH_W2) == UINT16_C(0x0020));
    for (uint8_t mode = UINT8_C(0); mode <= UINT8_C(3); ++mode) {
        const uint16_t first = mod_first_word(mode, UINT8_C(0));
        TI_C54X_TEST_CHECK(ti_c54x_resolve_qualified_slot(first, LD_W2) ==
                           TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
        TI_C54X_TEST_CHECK(ti_c54x_resolve_qualified_slot(first, STH_W2) !=
                           TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
        TI_C54X_TEST_CHECK(ti_c54x_resolve_qualified_slot(first, STH_W2) !=
                           TI_C54X_OPCODE_ID_UNKNOWN);
    }
}

static void
test_syn4_words_resolve_syn4_rows(void)
{
    for (uint8_t mode = UINT8_C(0); mode <= UINT8_C(3); ++mode) {
        const uint16_t first = mod_first_word(mode, UINT8_C(0));
        TI_C54X_TEST_CHECK(ti_c54x_resolve_qualified_slot(
                               first, UINT16_C(0x0C00)) ==
                           TI_C54X_OPCODE_ID_ADD_SMEM_SHIFT);
        TI_C54X_TEST_CHECK(ti_c54x_resolve_qualified_slot(
                               first, UINT16_C(0x0C20)) ==
                           TI_C54X_OPCODE_ID_SUB_SMEM_SHIFT);
    }
}

static void
test_other_second_words_decline(void)
{
    static const uint16_t declined[] = {
        UINT16_C(0x0000),
        UINT16_C(0x0C80), UINT16_C(0x0CE0), UINT16_C(0xFFFF)};
    for (uint8_t mode = UINT8_C(0); mode <= UINT8_C(3); ++mode) {
        const uint16_t first = mod_first_word(mode, UINT8_C(0));
        for (size_t index = 0U;
             index < sizeof(declined) / sizeof(declined[0]); ++index) {
            TI_C54X_TEST_CHECK(
                ti_c54x_resolve_qualified_slot(first, declined[index]) ==
                TI_C54X_OPCODE_ID_UNKNOWN);
        }
    }
    TI_C54X_TEST_CHECK(ti_c54x_resolve_qualified_slot(UINT16_C(0x0000),
                                                     UINT16_C(0x0000)) ==
                       TI_C54X_OPCODE_ID_UNKNOWN);
}

int
main(void)
{
    test_slot_claimed();
    test_ld_word_resolves_ld();
    test_sth_word_resolves_mod_row();
    test_parity_bit_selects_family();
    test_syn4_words_resolve_syn4_rows();
    test_other_second_words_decline();
    return 0;
}
