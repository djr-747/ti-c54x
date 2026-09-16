/*
 * Module contract
 *
 * Owns: executable proof that all 255 non-shadow RC first words
 * classify through the generic decoder, copy their complete
 * single-word bundles and scalar metadata, extract the
 * condition byte, pin the RET shadow word and the RETD
 * delayed-bit word outside this row, audit fixed-bit near-miss
 * siblings, and reject malformed inputs transactionally.
 * Does not own: catalog generation, condition evaluation, stack
 * reads, CPU/state mutation, fetch or bus behavior, sequencing,
 * timing, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t RC_VALUE = UINT16_C(0xFC00);

static uint16_t
first_word(uint8_t cond)
{
    return (uint16_t)(RC_VALUE | cond);
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
check_case(uint8_t cond)
{
    const uint16_t word = first_word(cond);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    /* The shadow word decodes as RET, never as RC. */
    if (cond == UINT8_C(0)) {
        TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RET);
        return;
    }
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_RC);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_RC);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_COND8);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_C_TC_STACK_POINTER);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(1));

    /* The single layout operand is the condition byte. */
    check_operand(&view, &words, UINT8_C(0), cond, UINT8_C(8));
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
        ti_c54x_decode_first_word(first_word(UINT8_C(0x30)));
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {first_word(UINT8_C(0x30))},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_RC);

    /* 0xFC00 is the RET shadow word: valid pattern shape,
     * wrong identity. */
    bundle.program_words[0] = RC_VALUE;
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
test_delayed_sibling_is_retd(void)
{
    /* Setting fixed bit 9 reaches the RETD word; RCD stays
     * deferred, so the delayed sibling is RETD, never RC. */
    const ti_c54x_decode_result result =
        ti_c54x_decode_first_word(
            (uint16_t)(RC_VALUE | UINT16_C(0x0200)));

    TI_C54X_TEST_CHECK(result.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_RETD);
}

static void
test_fixed_bit_near_misses(void)
{
    /* Mask 0xFF00 fixes bits 15:8 and leaves the condition byte
     * variable. Bits 0:7 stay inside this row (pinned by the
     * exhaustive loop, except the RET shadow word). Setting
     * fixed bit 9 lands on RETD (pinned above), clearing fixed
     * bit 10 lands on the BC sibling, and clearing fixed bit 11
     * lands on ADD_ACC_SHIFT; all other fixed-bit flips must
     * miss every row. */
    static const uint16_t unknown_masks[] = {
        UINT16_C(0x0100), UINT16_C(0x1000), UINT16_C(0x2000),
        UINT16_C(0x4000), UINT16_C(0x8000)};

    for (size_t index = 0U;
         index < sizeof(unknown_masks) / sizeof(unknown_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(RC_VALUE ^ unknown_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* Setting fixed bit 9 lands on RETD. */
    {
        const ti_c54x_decode_result retd_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RC_VALUE ^ UINT16_C(0x0200)));

        TI_C54X_TEST_CHECK(retd_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(retd_sibling.id == TI_C54X_OPCODE_ID_RETD);
    }

    /* Clearing fixed bit 10 lands on the BC sibling. */
    {
        const ti_c54x_decode_result bc_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RC_VALUE ^ UINT16_C(0x0400)));

        TI_C54X_TEST_CHECK(bc_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(bc_sibling.id == TI_C54X_OPCODE_ID_BC_PMAD);
    }

    /* Clearing fixed bit 11 lands on ADD_ACC_SHIFT. */
    {
        const ti_c54x_decode_result add_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(RC_VALUE ^ UINT16_C(0x0800)));

        TI_C54X_TEST_CHECK(add_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(add_sibling.id == TI_C54X_OPCODE_ID_ADD_ACC_SHIFT);
    }
}

int
main(void)
{
    /* Exhaustive over the condition byte, including the RET
     * shadow word. */
    for (uint32_t cond = 0U; cond <= UINT32_C(0xFF); ++cond) {
        check_case((uint8_t)cond);
    }
    test_malformed_inputs_reject_transactionally();
    test_delayed_sibling_is_retd();
    test_fixed_bit_near_misses();
    return 0;
}
