/*
 * Module contract
 *
 * Owns: executable proof that all 256 two-word BANZ first
 * words classify through the generic decoder, copy their complete
 * bundles and scalar metadata, extract the Sind byte then the raw
 * 16-bit pmad extension, pin the deferred delayed word to
 * UNKNOWN_WORD, audit fixed-bit near-miss siblings, and reject
 * malformed inputs transactionally.
 * Does not own: catalog generation, Sind evaluation, AR writes,
 * CPU/state mutation, fetch or bus behavior, sequencing, timing,
 * or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t BANZ_VALUE = UINT16_C(0x6C00);
/* Bit 9 set selects the deferred delayed BANZD variant, which keeps
 * mask bit 9 fixed at zero under 0xFF00. */
static const uint16_t BANZ_DELAYED_WORD = UINT16_C(0x6E00);

static uint16_t
first_word(uint8_t sind)
{
    return (uint16_t)(BANZ_VALUE | sind);
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
check_case(uint8_t sind, uint16_t pmad)
{
    const uint16_t word = first_word(sind);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word, pmad}, .program_word_count = UINT8_C(2)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_BANZ);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_BANZ);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_BRANCH_SIND_PMAD16);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_INELIGIBLE);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_AUXILIARY_REGISTERS);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == pmad);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(2));

    /* Layout order is Sind byte, pmad extension. */
    check_operand(&view, &words, UINT8_C(0), sind, UINT8_C(8));
    check_operand(&view, &words, UINT8_C(1), pmad, UINT8_C(16));
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
        ti_c54x_decode_first_word(first_word(UINT8_C(0x89)));
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {first_word(UINT8_C(0x89)), UINT16_C(0x2000)},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_BANZ);

    /* 0x6E00 is the deferred BANZD word: valid pattern shape, wrong
     * identity while deferred. */
    bundle.program_words[0] = BANZ_DELAYED_WORD;
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0x6000);
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
test_delayed_word_decodes_banzd(void)
{
    /* The W71 BANZD row claims the deferred delayed word this pin
     * held UNKNOWN through W70. */
    const ti_c54x_decode_result result =
        ti_c54x_decode_first_word(BANZ_DELAYED_WORD);

    TI_C54X_TEST_CHECK(result.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_BANZD);
}

static void
test_fixed_bit_near_misses(void)
{
    /* Mask 0xFF00 fixes bits 15:8 and leaves the Sind byte
     * variable. Setting fixed bit 9 lands on the deferred BANZD
     * word (pinned UNKNOWN above); all other fixed-bit flips must
     * miss every row. */
    static const uint16_t unknown_masks[] = {
        UINT16_C(0x0100),
        UINT16_C(0x0400), UINT16_C(0x0800), UINT16_C(0x1000),
        UINT16_C(0x2000), UINT16_C(0x4000), UINT16_C(0x8000)};

    for (size_t index = 0U;
         index < sizeof(unknown_masks) / sizeof(unknown_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(BANZ_VALUE ^ unknown_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }
}

int
main(void)
{
    static const uint16_t pmads[] = {
        UINT16_C(0x0000), UINT16_C(0x0002), UINT16_C(0x1000),
        UINT16_C(0x2000), UINT16_C(0xFFFF)};

    /* Exhaustive over the Sind byte for representative targets. */
    for (uint32_t sind = 0U; sind <= UINT32_C(0xFF); ++sind) {
        for (size_t index = 0U;
             index < sizeof(pmads) / sizeof(pmads[0]); ++index) {
            check_case((uint8_t)sind, pmads[index]);
        }
    }
    test_malformed_inputs_reject_transactionally();
    test_delayed_word_decodes_banzd();
    test_fixed_bit_near_misses();
    return 0;
}
