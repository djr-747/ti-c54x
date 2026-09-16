/*
 * Module contract
 *
 * Owns: executable proof that the single two-word BD first word
 * classifies through the generic decoder, copies its complete bundle
 * and scalar metadata, extracts the raw 16-bit pmad extension, pins
 * the non-delayed B_PMAD sibling word, audits fixed-bit
 * near-miss siblings, and rejects malformed inputs
 * transactionally.
 * Does not own: catalog generation, PC writes, CPU/state mutation,
 * fetch or bus behavior, sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t BD_VALUE = UINT16_C(0xF273);
/* Bit 9 clear selects the non-delayed B_PMAD sibling row. */
static const uint16_t B_PMAD_WORD = UINT16_C(0xF073);

static void
check_operand(const ti_c54x_operand_layout_set_view *view,
              const ti_c54x_program_words_view *words, uint8_t index,
              uint64_t expected, uint8_t width)
{
    ti_c54x_operand_field_value value = {
        UINT64_C(0xDEADBEEF), UINT8_C(0xA5)};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           view, words, index, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(value.raw_value == expected);
    TI_C54X_TEST_CHECK(value.width_bits == width);
}

static void
check_case(uint16_t pmad)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(BD_VALUE);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {BD_VALUE, pmad},
        .program_word_count = UINT8_C(2)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_BD);
    TI_C54X_TEST_CHECK(first.first_word == BD_VALUE);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_BD);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_PMAD16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       BD_VALUE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == pmad);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(1));

    /* Layout order is pmad only. */
    check_operand(&view, &words, UINT8_C(0), pmad, UINT8_C(16));
}

static void
check_transactional_failure(
    const ti_c54x_decode_result *first,
    const ti_c54x_instruction_word_bundle *bundle,
    ti_c54x_instruction_completion_status expected)
{
    ti_c54x_decoded_instruction output;
    unsigned char before[sizeof(output)];

    (void)memset(&output, 0xA5, sizeof(output));
    (void)memcpy(before, &output, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           first, bundle, &output) == expected);
    TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
}

static void
test_malformed_inputs_reject_transactionally(void)
{
    const ti_c54x_decode_result valid =
        ti_c54x_decode_first_word(BD_VALUE);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {BD_VALUE, UINT16_C(0x2000)},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_BD);

    /* 0xF073 is the B_PMAD sibling: valid row, wrong identity. */
    bundle.program_words[0] = B_PMAD_WORD;
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0xF000);
    bundle.program_words[0] = forged.first_word;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);

    forged = valid;
    forged.program_word_count = UINT8_C(1);
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH);

    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_UNKNOWN;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_UNKNOWN_OPCODE_ID);
}

static void
test_non_delayed_sibling(void)
{
    /* Clearing bit 9 selects the non-delayed B_PMAD row. */
    const ti_c54x_decode_result result =
        ti_c54x_decode_first_word(B_PMAD_WORD);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_B_PMAD);
}

static void
test_fixed_bit_near_misses(void)
{
    /* Mask 0xFFFF leaves no operand bit variable. Clearing fixed bit
     * 9 lands on the B_PMAD sibling, flipping fixed bit 11 lands
     * on the W69 BCD sibling, flipping fixed bit 7 lands
     * on SFTL_ACC, flipping fixed bit 6 lands on
     * AND_IMMEDIATE_SHIFT, flipping fixed bit 5 lands on
     * XOR_IMMEDIATE_SHIFT, and flipping fixed bit 4 lands on
     * AND_IMMEDIATE_SHIFT16, and flipping fixed bit 14 lands on the
     * P3-S3g MAC_DUAL_M1_M0 sibling; all other fixed-bit flips must
     * miss every row. */
    static const uint16_t unknown_masks[] = {
        UINT16_C(0x0001), UINT16_C(0x0002), UINT16_C(0x0004),
        UINT16_C(0x0008), UINT16_C(0x0100), UINT16_C(0x0400),
        UINT16_C(0x1000), UINT16_C(0x2000),
        UINT16_C(0x8000)};
    const ti_c54x_decode_result b_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(BD_VALUE ^ UINT16_C(0x0200)));
    const ti_c54x_decode_result sftl_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(BD_VALUE ^ UINT16_C(0x0080)));
    const ti_c54x_decode_result and_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(BD_VALUE ^ UINT16_C(0x0040)));
    const ti_c54x_decode_result xor_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(BD_VALUE ^ UINT16_C(0x0020)));
    const ti_c54x_decode_result and16_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(BD_VALUE ^ UINT16_C(0x0010)));

    TI_C54X_TEST_CHECK(b_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(b_sibling.id == TI_C54X_OPCODE_ID_B_PMAD);
    {
        const ti_c54x_decode_result bcd_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(BD_VALUE ^ UINT16_C(0x0800)));

        TI_C54X_TEST_CHECK(bcd_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(bcd_sibling.id == TI_C54X_OPCODE_ID_BCD);
    }
    TI_C54X_TEST_CHECK(sftl_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(sftl_sibling.id == TI_C54X_OPCODE_ID_SFTL_ACC);
    TI_C54X_TEST_CHECK(and_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(and_sibling.id ==
                       TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT);
    TI_C54X_TEST_CHECK(xor_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(xor_sibling.id ==
                       TI_C54X_OPCODE_ID_XOR_IMMEDIATE_SHIFT);
    TI_C54X_TEST_CHECK(and16_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(and16_sibling.id ==
                       TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT16);
    /* 0xB273 (fixed bit 14 set) is the MAC_DUAL_M1_M0 encoding with
     * R=0, S=1 (src B), D=0 (dst A), XMOD1 XAR=AR5, YMOD0 YAR=AR5. */
    const ti_c54x_decode_result mac_dual_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(BD_VALUE ^ UINT16_C(0x4000)));
    TI_C54X_TEST_CHECK(mac_dual_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(mac_dual_sibling.id == TI_C54X_OPCODE_ID_MAC_DUAL_M1_M0);

    for (size_t index = 0U;
         index < sizeof(unknown_masks) / sizeof(unknown_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(BD_VALUE ^ unknown_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }
}

int
main(void)
{
    static const uint16_t pmads[] = {
        UINT16_C(0x0000), UINT16_C(0x0001), UINT16_C(0x0002),
        UINT16_C(0x00FF), UINT16_C(0x1F45), UINT16_C(0x2000),
        UINT16_C(0x7FFF), UINT16_C(0x8000), UINT16_C(0xFEDC),
        UINT16_C(0xFFFE), UINT16_C(0xFFFF)};

    for (size_t index = 0U;
         index < sizeof(pmads) / sizeof(pmads[0]); ++index) {
        check_case(pmads[index]);
    }
    test_malformed_inputs_reject_transactionally();
    test_non_delayed_sibling();
    test_fixed_bit_near_misses();
    return 0;
}
