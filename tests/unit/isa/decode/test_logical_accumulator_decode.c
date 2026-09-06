/*
 * Module contract
 *
 * Owns: exhaustive first-word classification for the planned AND/OR/XOR
 * accumulator rows, copied catalog metadata, and extraction of source,
 * destination, and raw signed-five-bit shift fields. It also proves that
 * forged first-word/bundle inputs reject transactionally.
 * Does not own: logical operation semantics, CPU/state mutation, sequencing,
 * timing, generated lookup production, or semantic handler implementation.
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

typedef struct logical_case {
    ti_c54x_opcode_id id;
    uint16_t value;
} logical_case;

static const logical_case cases[] = {
    {TI_C54X_OPCODE_ID_AND_ACC_SHIFT, UINT16_C(0xF080)},
    {TI_C54X_OPCODE_ID_OR_ACC_SHIFT, UINT16_C(0xF0A0)},
    {TI_C54X_OPCODE_ID_XOR_ACC_SHIFT, UINT16_C(0xF0C0)},
};

static const uint16_t logical_mask = UINT16_C(0xFCE0);

static int8_t
sign_extend_shift5(uint8_t raw)
{
    raw &= UINT8_C(0x1F);
    return (int8_t)((raw & UINT8_C(0x10)) != UINT8_C(0)
                        ? (int)raw - 32
                        : (int)raw);
}

static bool
is_logical_id(ti_c54x_opcode_id id)
{
    return id == TI_C54X_OPCODE_ID_AND_ACC_SHIFT ||
           id == TI_C54X_OPCODE_ID_OR_ACC_SHIFT ||
           id == TI_C54X_OPCODE_ID_XOR_ACC_SHIFT;
}

static ti_c54x_opcode_id
expected_id(uint16_t word)
{
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]);
         ++index) {
        if ((word & logical_mask) == cases[index].value) {
            return cases[index].id;
        }
    }
    return TI_C54X_OPCODE_ID_UNKNOWN;
}

static void
test_exhaustive_classification_and_no_collision(void)
{
    unsigned new_match_count = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_opcode_id expected = expected_id(word);
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);

        TI_C54X_TEST_CHECK(decoded.id == expected ||
                           (!is_logical_id(decoded.id) &&
                            expected == TI_C54X_OPCODE_ID_UNKNOWN));
        if (expected != TI_C54X_OPCODE_ID_UNKNOWN) {
            ++new_match_count;
            TI_C54X_TEST_CHECK(decoded.status == TI_C54X_DECODE_STATUS_OK);
            TI_C54X_TEST_CHECK(decoded.id == expected);
            TI_C54X_TEST_CHECK(decoded.first_word == word);
            TI_C54X_TEST_CHECK(decoded.program_word_count == UINT8_C(1));
        } else {
            TI_C54X_TEST_CHECK(!is_logical_id(decoded.id));
        }
    }
    TI_C54X_TEST_CHECK(new_match_count == 384U);
}

static void
test_field_extraction(const logical_case *test_case)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(test_case->id);

    for (uint16_t source_bit = 0U; source_bit < UINT16_C(2);
         ++source_bit) {
        for (uint16_t destination_bit = 0U;
             destination_bit < UINT16_C(2); ++destination_bit) {
            for (uint16_t raw_shift = 0U; raw_shift < UINT16_C(32);
                 ++raw_shift) {
                const uint16_t word = (uint16_t)(
                    test_case->value | (uint16_t)(source_bit << 9U) |
                    (uint16_t)(destination_bit << 8U) | raw_shift);
                const ti_c54x_decode_result first =
                    ti_c54x_decode_first_word(word);
                const ti_c54x_instruction_word_bundle bundle = {
                    .program_words = {word, UINT16_C(0)},
                    .program_word_count = UINT8_C(1)};
                const ti_c54x_program_words_view words = {
                    bundle.program_words, bundle.program_word_count};
                ti_c54x_decoded_instruction instruction;
                ti_c54x_operand_layout_set_view view;
                ti_c54x_operand_field_value source = {0U, 0U};
                ti_c54x_operand_field_value destination = {0U, 0U};
                ti_c54x_operand_field_value shift = {0U, 0U};

                TI_C54X_TEST_CHECK(metadata != NULL);
                TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
                TI_C54X_TEST_CHECK(first.id == test_case->id);
                TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                                       &first, &bundle, &instruction) ==
                                   TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
                TI_C54X_TEST_CHECK(instruction.id == metadata->id);
                TI_C54X_TEST_CHECK(
                    instruction.operand_layout ==
                    TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
                TI_C54X_TEST_CHECK(instruction.repeatability ==
                                   metadata->repeatability);
                TI_C54X_TEST_CHECK(instruction.input_resources ==
                                   metadata->input_resources);
                TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                                       instruction.operand_layout, UINT8_C(1),
                                       &view) ==
                                   TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                                       &view, &words, UINT8_C(0), &source) ==
                                   TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                                       &view, &words, UINT8_C(1), &destination) ==
                                   TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                                       &view, &words, UINT8_C(2), &shift) ==
                                   TI_C54X_OPERAND_LAYOUT_STATUS_OK);
                TI_C54X_TEST_CHECK(source.width_bits == UINT8_C(1));
                TI_C54X_TEST_CHECK(destination.width_bits == UINT8_C(1));
                TI_C54X_TEST_CHECK(shift.width_bits == UINT8_C(5));
                TI_C54X_TEST_CHECK(source.raw_value == ((word >> 9U) & 1U));
                TI_C54X_TEST_CHECK(destination.raw_value ==
                                   ((word >> 8U) & 1U));
                TI_C54X_TEST_CHECK(shift.raw_value ==
                                   (word & UINT16_C(0x001F)));
                {
                    const uint8_t extracted_raw_shift =
                        (uint8_t)(word & UINT16_C(0x001F));
                    const int8_t expected_shift =
                        (extracted_raw_shift & UINT8_C(0x10)) != UINT8_C(0)
                            ? (int8_t)((int)extracted_raw_shift - 32)
                            : (int8_t)extracted_raw_shift;
                    TI_C54X_TEST_CHECK(
                        sign_extend_shift5(extracted_raw_shift) ==
                        expected_shift);
                }
            }
        }
    }
}

static void
check_failure(const ti_c54x_decode_result *first,
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
test_forged_inputs_reject_transactionally(void)
{
    const uint16_t word = UINT16_C(0xF080);
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    forged.status = TI_C54X_DECODE_STATUS_UNKNOWN_WORD;
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_NOT_OK);

    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_UNKNOWN;
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_UNKNOWN_OPCODE_ID);

    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_COUNT;
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_INVALID_OPCODE_ID);

    forged = valid;
    forged.program_word_count = UINT8_C(0);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_INVALID);

    forged = valid;
    forged.program_word_count = UINT8_C(2);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH);

    bundle.program_word_count = UINT8_C(0);
    check_failure(&valid, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_INVALID);

    bundle = valid_bundle;
    bundle.program_word_count = UINT8_C(2);
    check_failure(&valid, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);

    bundle = valid_bundle;
    bundle.program_words[0] = UINT16_C(0xF081);
    check_failure(&valid, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_MISMATCH);

    bundle = valid_bundle;
    forged = valid;
    forged.first_word = UINT16_C(0xF000);
    bundle.program_words[0] = UINT16_C(0xF000);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);

    /* A canonical word cannot be rebound to a different family identity. */
    bundle = valid_bundle;
    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_OR_ACC_SHIFT;
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);
}

int
main(void)
{
    test_exhaustive_classification_and_no_collision();
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]);
         ++index) {
        test_field_extraction(&cases[index]);
    }
    test_forged_inputs_reject_transactionally();
    return 0;
}
