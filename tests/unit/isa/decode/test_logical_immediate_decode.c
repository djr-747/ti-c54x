/*
 * Module contract
 *
 * Owns: executable proof that all six two-word logical-immediate
 * forms classify through the generic decoder, copy their complete bundles and
 * scalar metadata, and extract S, D, SHFT4 (when present), and unsigned raw
 * lk16 from word 1. It also proves malformed first-result/bundle inputs reject
 * transactionally.
 * Does not own: logical operation semantics, CPU/state mutation, fetch or bus
 * behavior, sequencing, timing, generated lookup production, or handler bodies.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct logical_immediate_case {
    ti_c54x_opcode_id id;
    uint16_t value;
    uint16_t mask;
    bool has_shift;
} logical_immediate_case;

static const logical_immediate_case cases[] = {
    {TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT, UINT16_C(0xF030),
     UINT16_C(0xFCF0), true},
    {TI_C54X_OPCODE_ID_AND_IMMEDIATE_SHIFT16, UINT16_C(0xF063),
     UINT16_C(0xFCFF), false},
    {TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT, UINT16_C(0xF040),
     UINT16_C(0xFCF0), true},
    {TI_C54X_OPCODE_ID_OR_IMMEDIATE_SHIFT16, UINT16_C(0xF064),
     UINT16_C(0xFCFF), false},
    {TI_C54X_OPCODE_ID_XOR_IMMEDIATE_SHIFT, UINT16_C(0xF050),
     UINT16_C(0xFCF0), true},
    {TI_C54X_OPCODE_ID_XOR_IMMEDIATE_SHIFT16, UINT16_C(0xF065),
     UINT16_C(0xFCFF), false},
};

static ti_c54x_operand_layout_id
expected_layout(const logical_immediate_case *test_case)
{
    return test_case->has_shift
               ? TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT4_EXTENSION16
               : TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_EXTENSION16_SHIFT16;
}

static void
check_operand(const ti_c54x_operand_layout_set_view *view,
              const ti_c54x_program_words_view *words,
              uint8_t index,
              uint64_t expected,
              uint8_t width)
{
    ti_c54x_operand_field_value value = {UINT64_C(0xDEADBEEF), UINT8_C(0xA5)};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           view, words, index, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(value.raw_value == expected);
    TI_C54X_TEST_CHECK(value.width_bits == width);
}

static void
test_complete_decode_and_generic_layout_extraction(
    const logical_immediate_case *test_case)
{
    const uint16_t extension_values[] = {
        UINT16_C(0x0000), UINT16_C(0x8000), UINT16_C(0xFFFF)};

    for (uint16_t source = 0U; source < UINT16_C(2); ++source) {
        for (uint16_t destination = 0U; destination < UINT16_C(2);
             ++destination) {
            for (uint16_t shift = 0U;
                 test_case->has_shift && shift < UINT16_C(16);
                 ++shift) {
                const uint16_t first_word = (uint16_t)(
                    test_case->value | (uint16_t)(source << 9U) |
                    (uint16_t)(destination << 8U) | shift);

                for (size_t extension_index = 0U;
                     extension_index < sizeof(extension_values) /
                                            sizeof(extension_values[0]);
                     ++extension_index) {
                    const uint16_t extension =
                        extension_values[extension_index];
                    const ti_c54x_decode_result first =
                        ti_c54x_decode_first_word(first_word);
                    const ti_c54x_instruction_word_bundle bundle = {
                        .program_words = {first_word, extension},
                        .program_word_count = UINT8_C(2)};
                    const ti_c54x_program_words_view words = {
                        bundle.program_words, bundle.program_word_count};
                    ti_c54x_decoded_instruction instruction;
                    ti_c54x_operand_layout_set_view view;

                    TI_C54X_TEST_CHECK(first.status ==
                                       TI_C54X_DECODE_STATUS_OK);
                    TI_C54X_TEST_CHECK(first.id == test_case->id);
                    TI_C54X_TEST_CHECK(first.first_word == first_word);
                    TI_C54X_TEST_CHECK(first.program_word_count ==
                                       UINT8_C(2));
                    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                                           &first, &bundle, &instruction) ==
                                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
                    TI_C54X_TEST_CHECK(instruction.id == test_case->id);
                    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                                       expected_layout(test_case));
                    TI_C54X_TEST_CHECK(instruction.repeatability ==
                                       TI_C54X_REPEATABILITY_UNRESOLVED);
                    TI_C54X_TEST_CHECK(instruction.input_resources ==
                                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
                    TI_C54X_TEST_CHECK(
                        instruction.bundle.program_word_count == UINT8_C(2));
                    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                                       first_word);
                    TI_C54X_TEST_CHECK(instruction.bundle.program_words[1] ==
                                       extension);
                    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                           instruction.operand_layout,
                                           UINT8_C(2), &view) ==
                                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                    TI_C54X_TEST_CHECK(view.descriptor != NULL);
                    TI_C54X_TEST_CHECK(
                        view.descriptor->operand_count ==
                        (test_case->has_shift ? UINT8_C(4) : UINT8_C(3)));
                    check_operand(&view, &words, UINT8_C(0), source,
                                  UINT8_C(1));
                    check_operand(&view, &words, UINT8_C(1), destination,
                                  UINT8_C(1));
                    if (test_case->has_shift) {
                        check_operand(&view, &words, UINT8_C(2), shift,
                                      UINT8_C(4));
                        check_operand(&view, &words, UINT8_C(3), extension,
                                      UINT8_C(16));
                    } else {
                        check_operand(&view, &words, UINT8_C(2), extension,
                                      UINT8_C(16));
                    }
                }
            }

            if (!test_case->has_shift) {
                const uint16_t first_word = (uint16_t)(
                    test_case->value | (uint16_t)(source << 9U) |
                    (uint16_t)(destination << 8U));
                const ti_c54x_decode_result first =
                    ti_c54x_decode_first_word(first_word);
                const ti_c54x_instruction_word_bundle bundle = {
                    .program_words = {first_word, UINT16_C(0xFFFF)},
                    .program_word_count = UINT8_C(2)};
                const ti_c54x_program_words_view words = {
                    bundle.program_words, bundle.program_word_count};
                ti_c54x_decoded_instruction instruction;
                ti_c54x_operand_layout_set_view view;

                TI_C54X_TEST_CHECK(first.id == test_case->id);
                TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                                       &first, &bundle, &instruction) ==
                                   TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                       instruction.operand_layout, UINT8_C(2),
                                       &view) ==
                                   TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                check_operand(&view, &words, UINT8_C(2), UINT16_C(0xFFFF),
                              UINT8_C(16));
            }
        }
    }
}

static void
check_failure(const ti_c54x_decode_result *first,
              const ti_c54x_instruction_word_bundle *bundle,
              ti_c54x_instruction_completion_status expected)
{
    unsigned char before[sizeof(ti_c54x_decoded_instruction)];
    ti_c54x_decoded_instruction output;

    (void)memset(&output, 0xA5, sizeof(output));
    (void)memcpy(before, &output, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           first, bundle, &output) == expected);
    TI_C54X_TEST_CHECK(memcmp(before, &output, sizeof(output)) == 0);
}

static void
test_malformed_two_word_inputs_reject_transactionally(void)
{
    const uint16_t first_word = UINT16_C(0xF030);
    const ti_c54x_decode_result valid =
        ti_c54x_decode_first_word(first_word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {first_word, UINT16_C(0x8000)},
        .program_word_count = UINT8_C(2)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    forged.status = TI_C54X_DECODE_STATUS_UNKNOWN_WORD;
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_NOT_OK);

    forged = valid;
    forged.program_word_count = UINT8_C(1);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH);

    forged = valid;
    forged.first_word = UINT16_C(0xF031);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    forged = valid;
    bundle.program_word_count = UINT8_C(1);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_UNKNOWN;
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_UNKNOWN_OPCODE_ID);
}

static void
test_near_miss_words_do_not_decode_as_immediates(void)
{
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        const logical_immediate_case *test_case = &cases[index];
        const uint16_t fixed_bits = test_case->mask;

        for (unsigned bit = 0U; bit < 16U; ++bit) {
            const uint16_t bit_mask = (uint16_t)(UINT16_C(1) << bit);

            if ((fixed_bits & bit_mask) != 0U) {
                const ti_c54x_decode_result decoded =
                    ti_c54x_decode_first_word(
                        (uint16_t)(test_case->value ^ bit_mask));
                TI_C54X_TEST_CHECK(decoded.id != test_case->id);
            }
        }
    }
}

int
main(void)
{
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        test_complete_decode_and_generic_layout_extraction(&cases[index]);
    }
    test_malformed_two_word_inputs_reject_transactionally();
    test_near_miss_words_do_not_decode_as_immediates();
    return 0;
}
