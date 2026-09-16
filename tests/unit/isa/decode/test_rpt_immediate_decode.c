/*
 * Module contract
 *
 * Owns: executable COMMON_TI proof that the decode-owned immutable layout
 * set carries both immediate-RPT forms exactly as TI SPRU172C documents
 * them. Production first-word rows landed with the P3-S2 activation
 * slice and are pinned by the rpt_k/rpt_lk decode lanes.
 * Evidence: E-TI-C54X-SPRU172C-T2-16-P2-12-RPT-WIDTHS and
 * E-TI-C54X-SPRU172C-S4-RPT-P4-146-4-147.
 * Does not own: RPT catalog rows or first-word value/mask metadata
 * (pinned by the activation slice lanes), the RPT operation, repeat
 * state, CPU sequencing, timing, buses, or diagnostics.
 */

#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_field.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
check_extract_k(uint16_t first_word, uint16_t expected_k)
{
    const uint16_t words[] = {first_word, UINT16_C(0xFFFF)};
    const ti_c54x_program_words_view word_view = {words, 1U};
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value value = {0U, 0U};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K, 1U,
                           &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 0U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, expected_k);
    TI_C54X_TEST_CHECK(value.width_bits == 8U);
}

static void
check_extract_lk(uint16_t extension_word, uint16_t expected_lk)
{
    const uint16_t words[] = {UINT16_C(0xF070), extension_word};
    const ti_c54x_program_words_view word_view = {words, 2U};
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value value = {0U, 0U};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK, 2U,
                           &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 0U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, expected_lk);
    TI_C54X_TEST_CHECK(value.width_bits == 16U);
}

/* RPT #K is one program word with unsigned K in bits 7:0: the whole
 * 0xEC00-0xECFF neighborhood extracts its low byte as an 8-bit field. */
static void
test_k_neighborhood_exhaustive(void)
{
    uint32_t first_word;

    for (first_word = UINT32_C(0xEC00); first_word <= UINT32_C(0xECFF);
         ++first_word) {
        check_extract_k((uint16_t)first_word,
                        (uint16_t)(first_word & UINT32_C(0xFF)));
    }
}

/* RPT #lk is exact first word 0xF070 followed by one complete unsigned
 * 16-bit extension-word operand. */
static void
test_lk_extension_word_range(void)
{
    check_extract_lk(UINT16_C(0x0000), UINT16_C(0x0000));
    check_extract_lk(UINT16_C(0x0001), UINT16_C(0x0001));
    check_extract_lk(UINT16_C(0xBEEF), UINT16_C(0xBEEF));
    check_extract_lk(UINT16_C(0xFFFE), UINT16_C(0xFFFE));
    check_extract_lk(UINT16_C(0xFFFF), UINT16_C(0xFFFF));
}

static void
test_word_count_mismatch_rejected(void)
{
    ti_c54x_operand_layout_set_view view;

    memset(&view, 0xA5, sizeof(view));
    TI_C54X_TEST_CHECK(
        ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K, 2U, &view) ==
        TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH);
    TI_C54X_TEST_CHECK(
        ti_c54x_operand_layout_set_resolve(
            TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK, 1U, &view) ==
        TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH);
}

int
main(void)
{
    test_k_neighborhood_exhaustive();
    test_lk_extension_word_range();
    test_word_count_mismatch_rejected();
    puts("rpt_immediate_decode_common: ok");
    return 0;
}
