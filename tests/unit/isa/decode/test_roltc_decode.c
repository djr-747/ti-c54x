/*
 * Module contract
 *
 * Owns: executable proof of ROLTC first-word classification, copied
 * completion metadata, source-bit extraction, and transactional rejection of
 * malformed or stale completion inputs.
 * Does not own: rotate semantics, status mutation, CPU/state integration,
 * sequencing, timing, generated lookup production, or opcode registration.
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

typedef struct roltc_case {
    uint16_t word;
    ti_c54x_opcode_id id;
    uint8_t source;
} roltc_case;

static const roltc_case cases[] = {
    {UINT16_C(0xF492), TI_C54X_OPCODE_ID_ROLTC_ACC, UINT8_C(0)},
    {UINT16_C(0xF592), TI_C54X_OPCODE_ID_ROLTC_ACC, UINT8_C(1)},
};

static void
check_source_and_metadata(const roltc_case *test_case)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(test_case->id);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(test_case->word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {test_case->word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value source = {0U, 0U};

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == test_case->id);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == metadata->id);
    TI_C54X_TEST_CHECK(instruction.operand_layout == metadata->operand_layout);
    TI_C54X_TEST_CHECK(instruction.repeatability == metadata->repeatability);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       metadata->input_resources);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(0), &source) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(source.width_bits == UINT8_C(1));
    TI_C54X_TEST_CHECK(source.raw_value == test_case->source);
}

static void
test_exhaustive_classification_and_no_collision(void)
{
    unsigned matches = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const bool roltc = (word & UINT16_C(0xFEFF)) == UINT16_C(0xF492);
        const bool sftc = (word & UINT16_C(0xFEFF)) == UINT16_C(0xF494);
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);
        const bool new_row = decoded.id == TI_C54X_OPCODE_ID_ROLTC_ACC ||
                             decoded.id == TI_C54X_OPCODE_ID_SFTC_ACC;

        TI_C54X_TEST_CHECK(new_row == (roltc || sftc));
        if (roltc || sftc) {
            ++matches;
            TI_C54X_TEST_CHECK(decoded.status == TI_C54X_DECODE_STATUS_OK);
            TI_C54X_TEST_CHECK(decoded.id ==
                               (roltc ? TI_C54X_OPCODE_ID_ROLTC_ACC
                                      : TI_C54X_OPCODE_ID_SFTC_ACC));
            TI_C54X_TEST_CHECK(word == UINT16_C(0xF492) ||
                               word == UINT16_C(0xF592) ||
                               word == UINT16_C(0xF494) ||
                               word == UINT16_C(0xF594));
        }
    }
    TI_C54X_TEST_CHECK(matches == 4U);
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
test_malformed_and_stale_completion_is_transactional(void)
{
    const uint16_t word = UINT16_C(0xF492);
    const ti_c54x_decode_result valid = ti_c54x_decode_first_word(word);
    const ti_c54x_instruction_word_bundle valid_bundle = {
        .program_words = {word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decode_result forged = valid;
    ti_c54x_instruction_word_bundle bundle = valid_bundle;

    bundle.program_word_count = UINT8_C(0);
    check_failure(&valid, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_INVALID);

    bundle = valid_bundle;
    bundle.program_word_count = UINT8_C(2);
    check_failure(&valid, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_MISMATCH);

    bundle.program_word_count =
        (uint8_t)(TI_C54X_INSTRUCTION_WORD_BUNDLE_CAPACITY + UINT8_C(1));
    check_failure(&valid, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_BUNDLE_COUNT_INVALID);

    forged.program_word_count = UINT8_C(2);
    check_failure(&forged, &valid_bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_COUNT_MISMATCH);

    forged = valid;
    forged.id = TI_C54X_OPCODE_ID_SFTC_ACC;
    check_failure(&forged, &valid_bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);

    forged = valid;
    forged.first_word = UINT16_C(0xF494);
    bundle = valid_bundle;
    bundle.program_words[0] = UINT16_C(0xF494);
    check_failure(&forged, &bundle,
                  TI_C54X_INSTRUCTION_COMPLETION_STATUS_FIRST_WORD_PATTERN_MISMATCH);
}

static void
test_fixed_pattern_near_misses_are_rejected(void)
{
    static const uint16_t near_misses[] = {
        UINT16_C(0xF491), UINT16_C(0xF490), UINT16_C(0xF493),
        UINT16_C(0xF494), UINT16_C(0xF492) ^ UINT16_C(0x0001),
        UINT16_C(0xF492) ^ UINT16_C(0x0200)};

    for (size_t i = 0U; i < sizeof(near_misses) / sizeof(near_misses[0]);
         ++i) {
        const ti_c54x_opcode_id id =
            ti_c54x_decode_first_word(near_misses[i]).id;
        TI_C54X_TEST_CHECK(id != TI_C54X_OPCODE_ID_ROLTC_ACC);
    }
}

int
main(void)
{
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        check_source_and_metadata(&cases[i]);
    }
    test_exhaustive_classification_and_no_collision();
    test_malformed_and_stale_completion_is_transactional();
    test_fixed_pattern_near_misses_are_rejected();
    return 0;
}
