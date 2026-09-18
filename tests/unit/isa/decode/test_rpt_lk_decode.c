/*
 * Module contract
 *
 * Owns: executable proof that the exact RPT_LK first word
 * classifies through the generic decoder, copies its complete
 * two-word bundle and scalar metadata, extracts the 16-bit lk
 * operand, audits fixed-bit near-miss siblings, and rejects
 * malformed inputs transactionally.
 * Does not own: catalog generation, RC state, CPU sequencing,
 * fetch or bus behavior, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t RPT_LK_VALUE = UINT16_C(0xF070);

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
check_case(uint16_t immediate)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(RPT_LK_VALUE);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {RPT_LK_VALUE, immediate},
        .program_word_count = UINT8_C(2)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RPT_LK);
    TI_C54X_TEST_CHECK(first.first_word == RPT_LK_VALUE);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_RPT_LK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == RPT_LK_VALUE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == immediate);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(1));

    /* The layout carries the single lk operand. */
    check_operand(&view, &words, UINT8_C(0), immediate, UINT8_C(16));
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
        ti_c54x_decode_first_word(RPT_LK_VALUE);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {RPT_LK_VALUE, UINT16_C(0x1234)},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_RPT_LK);

    bundle.program_word_count = UINT8_C(1);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0xF071);
    bundle.program_words[0] = forged.first_word;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);

    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_UNKNOWN;
    check_transactional_failure(
        &forged, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_UNKNOWN_OPCODE_ID);
}

static void
test_fixed_bit_near_misses(void)
{
    /* The exact row fixes every first-word bit. Flipping bit 2
     * lands on the CALL_PMAD sibling, bit 5 on
     * XOR_IMMEDIATE_SHIFT, bit 6 on AND_IMMEDIATE_SHIFT, bit 7 on
     * SFTL_ACC, and bit 11 on BC_PMAD (each pinned below); bit 14
     * lands on the P3-S3g MAC_DUAL_M1_M0 sibling (pinned below); all
     * other single-bit flips must miss every row; flipping bit 4
     * lands on the P3-S27 ADD_LK_16 sibling (pinned below). */
    static const uint16_t unknown_masks[] = {
        UINT16_C(0x0001), UINT16_C(0x0002), UINT16_C(0x0008),
        UINT16_C(0x0100), UINT16_C(0x0200),
        UINT16_C(0x0400), UINT16_C(0x1000),
        UINT16_C(0x2000), UINT16_C(0x8000)};

    for (size_t index = 0U;
         index < sizeof(unknown_masks) / sizeof(unknown_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_LK_VALUE ^ unknown_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* 0xF060 (flipping bit 4) is the ADD_LK_16 encoding with S=0
     * (src A), D=0 (dst A): valid row, wrong identity. */
    {
        const ti_c54x_decode_result add_lk16_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_LK_VALUE ^ UINT16_C(0x0010)));

        TI_C54X_TEST_CHECK(add_lk16_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(add_lk16_sibling.id ==
                           TI_C54X_OPCODE_ID_ADD_LK_16);
    }

    /* 0xB070 (flipping bit 14) is the MAC_DUAL_M1_M0 encoding with
     * R=0, S=0 (src A), D=0 (dst A), XMOD1 XAR=AR5, YMOD0 YAR=AR2. */
    {
        const ti_c54x_decode_result mac_dual_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_LK_VALUE ^ UINT16_C(0x4000)));

        TI_C54X_TEST_CHECK(mac_dual_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mac_dual_sibling.id ==
                           TI_C54X_OPCODE_ID_MAC_DUAL_M1_M0);
    }

    /* Flipping bit 2 lands on the CALL_PMAD sibling. */
    {
        const ti_c54x_decode_result call_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_LK_VALUE ^ UINT16_C(0x0004)));

        TI_C54X_TEST_CHECK(call_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(call_sibling.id == TI_C54X_OPCODE_ID_CALL_PMAD);
    }

    /* Flipping bit 5 lands on XOR_IMMEDIATE_SHIFT. */
    {
        const ti_c54x_decode_result xor_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_LK_VALUE ^ UINT16_C(0x0020)));

        TI_C54X_TEST_CHECK(xor_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(xor_sibling.id ==
                           TI_C54X_OPCODE_ID_XOR_IMMEDIATE_SHIFT);
    }

    /* Flipping bit 6 lands on AND_IMMEDIATE_SHIFT. */
    {
        const ti_c54x_decode_result and_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_LK_VALUE ^ UINT16_C(0x0040)));

        TI_C54X_TEST_CHECK(and_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(and_sibling.id ==
                           TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT);
    }

    /* Flipping bit 7 lands on SFTL_ACC. */
    {
        const ti_c54x_decode_result sftl_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_LK_VALUE ^ UINT16_C(0x0080)));

        TI_C54X_TEST_CHECK(sftl_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sftl_sibling.id == TI_C54X_OPCODE_ID_SFTL_ACC);
    }

    /* Flipping bit 11 lands on BC_PMAD. */
    {
        const ti_c54x_decode_result bc_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RPT_LK_VALUE ^ UINT16_C(0x0800)));

        TI_C54X_TEST_CHECK(bc_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(bc_sibling.id == TI_C54X_OPCODE_ID_BC_PMAD);
    }
}

int
main(void)
{
    /* Immediate corners. */
    check_case(UINT16_C(0x0000));
    check_case(UINT16_C(0x0002));
    check_case(UINT16_C(0x7FFF));
    check_case(UINT16_C(0x8000));
    check_case(UINT16_C(0xFFFF));
    test_malformed_inputs_reject_transactionally();
    test_fixed_bit_near_misses();
    return 0;
}
