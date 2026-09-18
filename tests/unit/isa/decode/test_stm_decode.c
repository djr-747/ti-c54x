/*
 * Module contract
 *
 * Owns: executable proof that all 256 STM first words classify
 * through the generic decoder, copy their complete two-word
 * bundles and scalar metadata, extract the indirect flag,
 * 7-bit direct address, and 16-bit immediate, audit fixed-bit
 * near-miss siblings, and reject malformed inputs
 * transactionally.
 * Does not own: catalog generation, address resolution, MMR
 * routing, CPU/state mutation, fetch or bus behavior,
 * sequencing, timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t STM_VALUE = UINT16_C(0x7700);

static uint16_t
first_word(uint8_t suffix)
{
    return (uint16_t)(STM_VALUE | suffix);
}

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
check_case(uint8_t suffix, uint16_t immediate)
{
    const uint16_t word = first_word(suffix);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, immediate}, .program_word_count = UINT8_C(2)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_STM);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_STM);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_DIRECT7_EXTENSION16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_NONE);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == immediate);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(3));

    /* Layout operands are the indirect flag, address, immediate. */
    check_operand(&view, &words, UINT8_C(0), (uint64_t)(suffix >> 7),
                  UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), (uint64_t)(suffix & 0x7FU),
                  UINT8_C(7));
    check_operand(&view, &words, UINT8_C(2), immediate, UINT8_C(16));
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
        ti_c54x_decode_first_word(first_word(UINT8_C(0x06)));
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {first_word(UINT8_C(0x06)), UINT16_C(0x1234)},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_STM);

    bundle.program_words[1] = UINT16_C(0xFFFF);
    bundle.program_word_count = UINT8_C(1);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0x7606);
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
    /* Mask 0xFF00 fixes bits 15:8 and leaves the flag/address byte
     * variable (pinned by the exhaustive loop). Setting fixed
     * bit 15 lands on the ADD_ACC_SHIFT row (pinned below),
     * clearing fixed bit 14 lands on the MACA row (pinned below),
     * and clearing fixed bit 9 lands on the P3-S6 PORTW direct row
     * (pinned below; TI SPRU172C 4-130 pins PORTW at 0x75xx);
     * clearing fixed bit 12 lands on the P3-S33 MAC_SMEM_LK row
     * (pinned below); clearing fixed bit 8 lands on the P3-S36
     * ST_LK_SMEM row (pinned below); all other fixed-bit flips
     * must miss every row. */
    static const uint16_t unknown_masks[] = {
        UINT16_C(0x0400),
        UINT16_C(0x0800), UINT16_C(0x2000)};

    for (size_t index = 0U;
         index < sizeof(unknown_masks) / sizeof(unknown_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(STM_VALUE ^ unknown_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* Setting fixed bit 15 lands on the ADD_ACC_SHIFT sibling. */
    {
        const ti_c54x_decode_result add_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(STM_VALUE ^ UINT16_C(0x8000)));

        TI_C54X_TEST_CHECK(add_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(add_sibling.id ==
                           TI_C54X_OPCODE_ID_ADD_ACC_SHIFT);
    }

    /* This former near-miss is the P3-S33 MAC Smem,#lk sibling:
     * 0x7700 ^ 0x1000 = 0x6700 classifies into the MAC_SMEM_LK row. */
    {
        const ti_c54x_decode_result mac_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(STM_VALUE ^ UINT16_C(0x1000)));

        TI_C54X_TEST_CHECK(mac_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mac_sibling.id == TI_C54X_OPCODE_ID_MAC_SMEM_LK);
    }

    /* This former near-miss is the P3-S36 ST #lk,Smem sibling:
     * 0x7700 ^ 0x0100 = 0x7600 classifies into the ST_LK_SMEM row
     * (direct dmad 0). */
    {
        const ti_c54x_decode_result st_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(STM_VALUE ^ UINT16_C(0x0100)));

        TI_C54X_TEST_CHECK(st_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(st_sibling.id == TI_C54X_OPCODE_ID_ST_LK_SMEM);
    }

    /* This former near-miss is the P3-S3o MACA Smem sibling:
     * 0x7700 ^ 0x4000 = 0x3700 classifies into the MACA_SMEM row. */
    {
        const ti_c54x_decode_result maca_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(STM_VALUE ^ UINT16_C(0x4000)));

        TI_C54X_TEST_CHECK(maca_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(maca_sibling.id == TI_C54X_OPCODE_ID_MACA_SMEM);
    }

    /* 0x7700 ^ 0x0200 = 0x7500 classifies into the P3-S6 PORTW
     * direct row. */
    {
        const ti_c54x_decode_result portw_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(STM_VALUE ^ UINT16_C(0x0200)));

        TI_C54X_TEST_CHECK(portw_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(portw_sibling.id ==
                           TI_C54X_OPCODE_ID_PORTW_DIRECT);
    }
}

int
main(void)
{
    /* Exhaustive over the flag/address byte with a fixed immediate. */
    for (uint32_t suffix = 0U; suffix <= UINT32_C(0xFF); ++suffix) {
        check_case((uint8_t)suffix, UINT16_C(0xBEEF));
    }
    /* Immediate corners at a fixed direct word. */
    check_case(UINT8_C(0x06), UINT16_C(0x0000));
    check_case(UINT8_C(0x06), UINT16_C(0xFFFF));
    test_malformed_inputs_reject_transactionally();
    test_fixed_bit_near_misses();
    return 0;
}
