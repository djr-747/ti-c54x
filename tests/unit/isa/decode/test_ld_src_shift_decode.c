/*
 * Module contract
 *
 * Owns: executable proof that all single-word LD src[,SHIFT],dst first
 * words classify through the generic decoder, copy their complete bundles
 * and scalar metadata, extract source then destination then raw SHIFT5,
 * and reject malformed inputs transactionally.
 * Does not own: catalog generation, accumulator reads, CPU/state
 * mutation, fetch or bus behavior, sequencing, timing, or handler
 * bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t LD_SRC_SHIFT_VALUE = UINT16_C(0xF440);

static uint16_t
first_word(uint8_t source, uint8_t destination, uint8_t shift)
{
    return (uint16_t)(LD_SRC_SHIFT_VALUE |
                      (uint16_t)(((uint16_t)(source & UINT8_C(1))) << 9U) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (uint16_t)(shift & UINT8_C(0x1F)));
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
check_case(uint8_t source, uint8_t destination, uint8_t shift)
{
    const uint16_t word = first_word(source, destination, shift);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_SRC_SHIFT);
    TI_C54X_TEST_CHECK(first.first_word == word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_SRC_SHIFT);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS_STATUS_SXM_OVM);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(3));

    /* Layout order is source, destination, SHIFT5. */
    check_operand(&view, &words, UINT8_C(0), source, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), destination, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(2), shift, UINT8_C(5));
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
    const uint16_t word = first_word(UINT8_C(1), UINT8_C(0), UINT8_C(0x1F));
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word}, .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_LD_SRC_SHIFT);

    bundle.program_words[0] = UINT16_C(0xF400);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0xF478);
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
test_fixed_bit_near_misses_are_unknown(void)
{
    /* Mask 0xFCE0 leaves S(9), D(8), SHIFT(4:0) variable; flips of
     * genuinely fixed bits must miss every row. Bit 6 selects the ADD
     * sibling and bit 10 an OR-immediate sibling, so those flips are
     * covered by sibling rows instead. Bit 11 lands in the RC row
     * (pinned below), so it leaves this list as well. Bit 15 lands in
     * the PORTR direct row (TI SPRU172C 4-129 pins PORTR at 0x74xx),
     * pinned below as well. Bit 5 lands in the P3-S39 SFTA_ACC row
     * (pinned below), so it leaves this list as well. Bit 7 lands
     * in the P3-S105 TRAP_K row (pinned below), so it leaves this
     * list as well. */
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x1000), UINT16_C(0x2000)};

    for (size_t index = 0U;
         index < sizeof(fixed_bit_masks) / sizeof(fixed_bit_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SRC_SHIFT_VALUE ^ fixed_bit_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* Flipping fixed bit 11 lands on the RC sibling. */
    {
        const ti_c54x_decode_result rc_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SRC_SHIFT_VALUE ^ UINT16_C(0x0800)));

        TI_C54X_TEST_CHECK(rc_sibling.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(rc_sibling.id == TI_C54X_OPCODE_ID_RC);
    }

    /* Flipping fixed bit 7 lands on 0xF4C0, the TRAP_K base word. */
    {
        const ti_c54x_decode_result trap_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SRC_SHIFT_VALUE ^ UINT16_C(0x0080)));

        TI_C54X_TEST_CHECK(trap_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(trap_sibling.id == TI_C54X_OPCODE_ID_TRAP_K);
    }

    /* Flipping fixed bit 15 lands on the PORTR direct sibling. */
    {
        const ti_c54x_decode_result portr_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SRC_SHIFT_VALUE ^ UINT16_C(0x8000)));

        TI_C54X_TEST_CHECK(portr_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(portr_sibling.id ==
                           TI_C54X_OPCODE_ID_PORTR_DIRECT);
    }

    /* Flipping fixed bit 5 lands on 0xF460, the SFTA_ACC base word. */
    {
        const ti_c54x_decode_result sfta_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SRC_SHIFT_VALUE ^ UINT16_C(0x0020)));

        TI_C54X_TEST_CHECK(sfta_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(sfta_sibling.id == TI_C54X_OPCODE_ID_SFTA_ACC);
    }
}

int
main(void)
{
    static const uint8_t shifts[] = {UINT8_C(0x00), UINT8_C(0x0F),
                                     UINT8_C(0x10), UINT8_C(0x1F)};

    for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
        for (uint8_t destination = 0U; destination <= UINT8_C(1);
             ++destination) {
            for (size_t index = 0U;
                 index < sizeof(shifts) / sizeof(shifts[0]); ++index) {
                check_case(source, destination, shifts[index]);
            }
        }
    }
    test_malformed_inputs_reject_transactionally();
    test_fixed_bit_near_misses_are_unknown();
    return 0;
}
