/*
 * Module contract
 *
 * Owns: executable proof that the single one-word RET first word
 * classifies through the generic decoder, copies its complete bundle
 * and scalar metadata with no operand extraction, pins the delayed
 * RETD sibling word, audits fixed-bit near-miss siblings,
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

static const uint16_t RET_VALUE = UINT16_C(0xFC00);
/* Bit 9 set selects the RETD sibling row bound in W67B. */
static const uint16_t RET_DELAYED_WORD = UINT16_C(0xFE00);

static void
check_case(void)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(RET_VALUE);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {RET_VALUE},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RET);
    TI_C54X_TEST_CHECK(first.first_word == RET_VALUE);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_RET);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STACK_POINTER);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == RET_VALUE);
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
        ti_c54x_decode_first_word(RET_VALUE);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {RET_VALUE},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_RET);

    /* 0xF800 is the BC_PMAD sibling: valid row, wrong identity. */
    bundle.program_words[0] = UINT16_C(0xF800);
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
test_delayed_word_decodes_retd(void)
{
    /* The W67B RETD row claims the bit-9 word; the W66B UNKNOWN pin
     * updated here under the cross-slice hazard rule. */
    const ti_c54x_decode_result result =
        ti_c54x_decode_first_word(RET_DELAYED_WORD);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_RETD);
    TI_C54X_TEST_CHECK(result.first_word == RET_DELAYED_WORD);
    TI_C54X_TEST_CHECK(result.program_word_count == UINT8_C(1));
}

static void
test_fixed_bit_near_misses(void)
{
    /* Mask 0xFFFF leaves no operand bit variable. Flipping fixed bit 9
     * lands on the RETD sibling, flipping fixed bit 10 lands on the
     * BC_PMAD sibling, flipping fixed bit 11 lands on
     * ADD_ACC_SHIFT, and flipping fixed bit 12 lands on the RPT_K
     * row. Flipping any low-byte bit lands in the RC row
     * (pinned below); flipping fixed bit 8 lands on the P3-S4 XC
     * row (pinned below); all other fixed-bit flips must miss
     * every row; clearing fixed bit 14 lands on MAS_DUAL_M0_M0
     * (S3i claims the 0xB8xx neighborhood; the UNKNOWN loop drops
     * that mask under the cross-slice hazard rule); clearing
     * fixed bit 15 lands on the P3-S63 MVDP_DIRECT row
     * (pinned below). */
    static const uint16_t unknown_masks[] = {
        UINT16_C(0x2000)};
    static const uint16_t rc_masks[] = {
        UINT16_C(0x0001), UINT16_C(0x0002), UINT16_C(0x0004),
        UINT16_C(0x0008), UINT16_C(0x0010), UINT16_C(0x0020),
        UINT16_C(0x0040), UINT16_C(0x0080)};
    const ti_c54x_decode_result retd_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RET_VALUE ^ UINT16_C(0x0200)));
    const ti_c54x_decode_result bc_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RET_VALUE ^ UINT16_C(0x0400)));
    const ti_c54x_decode_result add_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RET_VALUE ^ UINT16_C(0x0800)));
    const ti_c54x_decode_result rpt_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RET_VALUE ^ UINT16_C(0x1000)));
    const ti_c54x_decode_result mvdp_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RET_VALUE ^ UINT16_C(0x8000)));
    const ti_c54x_decode_result xc_sibling =
        ti_c54x_decode_first_word(
            (uint16_t)(RET_VALUE ^ UINT16_C(0x0100)));

    TI_C54X_TEST_CHECK(retd_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(retd_sibling.id == TI_C54X_OPCODE_ID_RETD);
    TI_C54X_TEST_CHECK(xc_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(xc_sibling.id == TI_C54X_OPCODE_ID_XC);
    TI_C54X_TEST_CHECK(bc_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(bc_sibling.id == TI_C54X_OPCODE_ID_BC_PMAD);
    TI_C54X_TEST_CHECK(add_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(add_sibling.id == TI_C54X_OPCODE_ID_ADD_ACC_SHIFT);
    TI_C54X_TEST_CHECK(rpt_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(rpt_sibling.id == TI_C54X_OPCODE_ID_RPT_K);
    TI_C54X_TEST_CHECK(mvdp_sibling.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(mvdp_sibling.id == TI_C54X_OPCODE_ID_MVDP_DIRECT);

    for (size_t index = 0U;
         index < sizeof(unknown_masks) / sizeof(unknown_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(RET_VALUE ^ unknown_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* Flipping any low-byte bit lands in the RC row. */
    for (size_t index = 0U;
         index < sizeof(rc_masks) / sizeof(rc_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(RET_VALUE ^ rc_masks[index]));

        TI_C54X_TEST_CHECK(result.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_RC);
    }

    /* Clearing fixed bit 14 lands on the S3i MAS dual row. */
    {
        const ti_c54x_decode_result mas_dual_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RET_VALUE ^ UINT16_C(0x4000)));

        TI_C54X_TEST_CHECK(mas_dual_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mas_dual_sibling.id == TI_C54X_OPCODE_ID_MAS_DUAL_M0_M0);
    }
}

int
main(void)
{
    check_case();
    test_malformed_inputs_reject_transactionally();
    test_delayed_word_decodes_retd();
    test_fixed_bit_near_misses();
    return 0;
}
