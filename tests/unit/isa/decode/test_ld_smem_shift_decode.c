/*
 * Module contract
 *
 * Owns: executable proof that all two-word LD Smem[,SHIFT],dst first words
 * classify through the generic decoder, copy their complete bundles and
 * scalar metadata, extract D then ARF then MOD then raw SHIFT5, and reject
 * malformed inputs transactionally. The 0x6F80 slot is second-word
 * qualified, so completion keeps LD only for 0x0C40 words here; STH and
 * unmatched second words are covered by the qualified-slot decode test.
 * Does not own: catalog generation, address resolution, memory effects,
 * CPU/state mutation, fetch or bus behavior, sequencing, timing, or
 * handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t LD_SMEM_SHIFT_VALUE = UINT16_C(0x6F80);

static uint16_t
first_word(uint8_t mode, uint8_t arf)
{
    return (uint16_t)(LD_SMEM_SHIFT_VALUE |
                      (uint16_t)(((uint16_t)(mode & UINT8_C(3))) << 3U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static uint16_t
second_word(uint8_t destination, uint8_t shift)
{
    return (uint16_t)(UINT16_C(0x0C40) |
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
check_case(uint8_t mode, uint8_t arf, uint8_t destination, uint8_t shift)
{
    const uint16_t word0 = first_word(mode, arf);
    const uint16_t word1 = second_word(destination, shift);
    const ti_c54x_decode_result first = ti_c54x_decode_first_word(word0);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {word0, word1}, .program_word_count = UINT8_C(2)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
    TI_C54X_TEST_CHECK(first.first_word == word0);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD_SHIFT5);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_STATUS_SXM_OVM);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(2));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] == word0);
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] == word1);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(2), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor != NULL);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == UINT8_C(4));

    /* Layout order is D in word 1, ARF and MOD in word 0, SHIFT5 in
     * word 1. */
    check_operand(&view, &words, UINT8_C(0), destination, UINT8_C(1));
    check_operand(&view, &words, UINT8_C(1), arf, UINT8_C(3));
    check_operand(&view, &words, UINT8_C(2), mode, UINT8_C(2));
    check_operand(&view, &words, UINT8_C(3), shift, UINT8_C(5));
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
test_malformed_two_word_inputs_reject_transactionally(void)
{
    const uint16_t word0 = first_word(UINT8_C(2), UINT8_C(5));
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word0);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word0, second_word(UINT8_C(1), UINT8_C(0x1F))},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    TI_C54X_TEST_CHECK(valid.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(valid.id == TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);

    /* Incomplete extension bundle. */
    bundle.program_word_count = UINT8_C(1);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);

    /* Wrong first word in an otherwise complete bundle. */
    bundle = valid_bundle;
    bundle.program_words[0] = UINT16_C(0x6FA0);
    check_transactional_failure(
        &valid, &bundle,
        TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    /* Forged first-word result must not be accepted as this opcode. */
    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0x6FB8);
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
test_fixed_bit_near_misses_are_unknown(void)
{
    /* Flipping fixed bit 12 lands on the P3-S65 WRITA Smem row
     * (0x6F80 ^ 0x1000 = 0x7F80; pinned below). */
    static const uint16_t fixed_bit_masks[] = {
        UINT16_C(0x8000)};

    for (size_t index = 0U;
         index < sizeof(fixed_bit_masks) / sizeof(fixed_bit_masks[0]);
         ++index) {
        const ti_c54x_decode_result result =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SMEM_SHIFT_VALUE ^ fixed_bit_masks[index]));

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_DECODE_STATUS_UNKNOWN_WORD);
        TI_C54X_TEST_CHECK(result.id == TI_C54X_OPCODE_ID_UNKNOWN);
    }

    /* Flipping bit 5 lands on the STH syntax-4 MOD4 row; flipping bit 7
     * lands on the STH syntax-4 direct row. */
    {
        const ti_c54x_decode_result mod4_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SMEM_SHIFT_VALUE ^ UINT16_C(0x0020)));

        TI_C54X_TEST_CHECK(mod4_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mod4_sibling.id ==
                           TI_C54X_OPCODE_ID_STH_SHIFT_SMEM_MOD4);
    }
    {
        const ti_c54x_decode_result direct_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SMEM_SHIFT_VALUE ^ UINT16_C(0x0080)));

        TI_C54X_TEST_CHECK(direct_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(direct_sibling.id ==
                           TI_C54X_OPCODE_ID_STH_SHIFT_DIRECT);
    }

    /* Flipping fixed bit 8 lands on the W71 BANZD sibling. */
    {
        const ti_c54x_decode_result banzd_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SMEM_SHIFT_VALUE ^ UINT16_C(0x0100)));

        TI_C54X_TEST_CHECK(banzd_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(banzd_sibling.id == TI_C54X_OPCODE_ID_BANZD);
    }

    /* Flipping fixed bit 12 lands on the P3-S65 WRITA Smem
     * row (0x6F80 ^ 0x1000 = 0x7F80). */
    {
        const ti_c54x_decode_result writa_sibling =
            ti_c54x_decode_first_word(
                (uint16_t)(LD_SMEM_SHIFT_VALUE ^ UINT16_C(0x1000)));

        TI_C54X_TEST_CHECK(writa_sibling.status ==
                           TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(writa_sibling.id ==
                           TI_C54X_OPCODE_ID_WRITA_SMEM);
    }

    /* MOD3 classifies here; the semantic leaf rejects it. */
    {
        const ti_c54x_decode_result mod3 =
            ti_c54x_decode_first_word(first_word(UINT8_C(3), UINT8_C(0)));

        TI_C54X_TEST_CHECK(mod3.status == TI_C54X_DECODE_STATUS_OK);
        TI_C54X_TEST_CHECK(mod3.id == TI_C54X_OPCODE_ID_LD_SMEM_SHIFT);
    }
}

int
main(void)
{
    static const uint8_t shifts[] = {UINT8_C(0x00), UINT8_C(0x0F),
                                     UINT8_C(0x10), UINT8_C(0x1F)};

    for (uint8_t mode = 0U; mode < UINT8_C(4); ++mode) {
        for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
            for (uint8_t destination = 0U; destination <= UINT8_C(1);
                 ++destination) {
                for (size_t index = 0U;
                     index < sizeof(shifts) / sizeof(shifts[0]); ++index) {
                    check_case(mode, arf, destination, shifts[index]);
                }
            }
        }
    }
    test_malformed_two_word_inputs_reject_transactionally();
    test_fixed_bit_near_misses_are_unknown();
    return 0;
}
