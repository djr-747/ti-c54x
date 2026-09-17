/*
 * Module contract
 *
 * Owns: executable proof that the single one-word RETD first word
 * classifies through the generic decoder, copies its complete bundle
 * and scalar metadata with no operand extraction, pins the
 * non-delayed RET sibling, audits fixed-bit near-miss siblings,
 * and rejects malformed inputs transactionally.
 * Does not own: catalog generation, stack pops, CPU/state mutation,
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

static const uint16_t RETD_VALUE = UINT16_C(0xFE00);
/* Bit 9 clear selects the non-delayed RET sibling. */
static const uint16_t RET_WORD = UINT16_C(0xFC00);

static void
check_case(void)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(RETD_VALUE);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {RETD_VALUE},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RETD);
    TI_C54X_TEST_CHECK(first.first_word == RETD_VALUE);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_RETD);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == RETD_VALUE);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(0));
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
        ti_c54x_decode_first_word(RETD_VALUE);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {RETD_VALUE},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_RETD);

    /* 0xFC00 is the RET sibling: valid row, wrong identity. */
    bundle.program_words[0] = RET_WORD;
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
    forged.program_word_count = UINT8_C(2);
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
    /* Clearing bit 9 selects the non-delayed RET row. */
    const ti_c54x_decode_result result =
        ti_c54x_decode_first_word(RET_WORD);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_RET);
}

static void
test_fixed_bit_near_misses(void)
{
    /* Mask 0xFFFF leaves no operand bit variable. Flipping fixed bit 9
     * lands on the RET sibling, flipping fixed bit 11 lands on
     * ADD_ACC_SHIFT, and flipping fixed bit 10 lands on the W69 BCD
     * sibling; flipping any low-byte bit lands in the RCD row
     * (pinned below, 0xFE00 itself stays RETD); flipping fixed
     * bit 8 lands on the P3-S4 XC row (pinned below); all other
     * fixed-bit flips must miss every row, except clearing
     * fixed bit 14 lands on MAS_DUAL_M0_M0 (S3i claims the 0xB8xx
     * neighborhood; the UNKNOWN loop drops that mask under the
     * cross-slice hazard rule). */
    static const uint16_t unknown_masks[] = {
        UINT16_C(0x1000),
        UINT16_C(0x2000), UINT16_C(0x8000)};
    static const uint16_t rcd_masks[] = {
        UINT16_C(0x0001), UINT16_C(0x0002), UINT16_C(0x0004),
        UINT16_C(0x0008), UINT16_C(0x0010), UINT16_C(0x0020),
        UINT16_C(0x0040), UINT16_C(0x0080)};
    const ti_c54x_decode_result ret_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RETD_VALUE ^ UINT16_C(0x0200)));
    const ti_c54x_decode_result sub_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RETD_VALUE ^ UINT16_C(0x0800)));
    const ti_c54x_decode_result bcd_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RETD_VALUE ^ UINT16_C(0x0400)));
    const ti_c54x_decode_result xc_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RETD_VALUE ^ UINT16_C(0x0100)));

    TI_C54X_TEST_CHECK(ret_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(ret_sibling.id == TI_C54X_OPCODE_ID_RET);
    TI_C54X_TEST_CHECK(xc_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(xc_sibling.id == TI_C54X_OPCODE_ID_XC);
    TI_C54X_TEST_CHECK(sub_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(sub_sibling.id == TI_C54X_OPCODE_ID_ADD_ACC_SHIFT);
    TI_C54X_TEST_CHECK(bcd_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(bcd_sibling.id == TI_C54X_OPCODE_ID_BCD);

    /* Low-byte flips stay under the RCD mask while the exact
     * 0xFE00 word keeps its RETD identity (pinned in check_case). */
    for (size_t index = 0U;
         index < sizeof(rcd_masks) / sizeof(rcd_masks[0]);
         ++index) {
        const ti_c54x_decode_result rcd_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RETD_VALUE ^ rcd_masks[index]));

        TI_C54X_TEST_CHECK(rcd_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(rcd_sibling.id == TI_C54X_OPCODE_ID_RCD);
    }

    for (size_t index = 0U;
         index < sizeof(unknown_masks) / sizeof(unknown_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(RETD_VALUE ^ unknown_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* Clearing fixed bit 14 lands on the S3i MAS dual row. */
    {
        const ti_c54x_decode_result mas_dual_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RETD_VALUE ^ UINT16_C(0x4000)));

        TI_C54X_TEST_CHECK(mas_dual_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mas_dual_sibling.id == TI_C54X_OPCODE_ID_MAS_DUAL_M0_M0);
    }
}

int
main(void)
{
    check_case();
    test_malformed_inputs_reject_transactionally();
    test_non_delayed_sibling();
    test_fixed_bit_near_misses();
    return 0;
}
