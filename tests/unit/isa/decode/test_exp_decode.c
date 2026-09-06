/*
 * Module contract
 *
 * Owns: executable proof that both EXP selected-accumulator first words
 * classify to one generic opcode identity, copy one-word completion metadata,
 * and extract selector S through the existing accumulator-source layout.
 * Does not own: EXP arithmetic, T-register effects, CPU/state mutation,
 * sequencing, timing, reset-runnable scenario accounting, generated lookup
 * production, or semantic handler implementation.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

typedef struct exp_case {
    uint16_t word;
    uint8_t source;
} exp_case;

static const exp_case cases[] = {
    {UINT16_C(0xF48E), UINT8_C(0)},
    {UINT16_C(0xF58E), UINT8_C(1)},
};

static void
test_selected_accumulator_decode(const exp_case *test_case)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(TI_C54X_OPCODE_ID_EXP_ACC);
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
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_EXP_ACC);
    TI_C54X_TEST_CHECK(first.first_word == test_case->word);
    TI_C54X_TEST_CHECK(first.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.id == TI_C54X_OPCODE_ID_EXP_ACC);
    TI_C54X_TEST_CHECK(instruction.bundle.program_word_count == UINT8_C(1));
    TI_C54X_TEST_CHECK(instruction.bundle.program_words[0] ==
                       test_case->word);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(instruction.repeatability ==
                       TI_C54X_REPEATABILITY_UNRESOLVED);
    TI_C54X_TEST_CHECK(instruction.input_resources ==
                       TI_C54X_INPUT_RESOURCES_ACCUMULATORS);
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
test_fixed_pattern_near_misses_are_not_exp(void)
{
    /* Avoid the selector bit and nearby rows while probing the fixed pattern. */
    static const uint16_t near_misses[] = {
        UINT16_C(0xF48F), UINT16_C(0xF48C), UINT16_C(0xF48A),
        UINT16_C(0xF49E), UINT16_C(0xF4CE), UINT16_C(0xF40E),
        UINT16_C(0xFC8E)};

    for (size_t index = 0U;
         index < sizeof(near_misses) / sizeof(near_misses[0]); ++index) {
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(near_misses[index]);
        TI_C54X_TEST_CHECK(decoded.id != TI_C54X_OPCODE_ID_EXP_ACC);
    }
}

static void
test_exhaustive_exp_classification(void)
{
    unsigned matches = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const int matches_exp =
            (word & UINT16_C(0xFEFF)) == UINT16_C(0xF48E);
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);

        if (matches_exp) {
            ++matches;
            TI_C54X_TEST_CHECK(decoded.status == TI_C54X_DECODE_STATUS_OK);
            TI_C54X_TEST_CHECK(decoded.id == TI_C54X_OPCODE_ID_EXP_ACC);
            TI_C54X_TEST_CHECK(decoded.program_word_count == UINT8_C(1));
        } else {
            TI_C54X_TEST_CHECK(decoded.id != TI_C54X_OPCODE_ID_EXP_ACC);
        }
    }
    TI_C54X_TEST_CHECK(matches == 2U);
}

int
main(void)
{
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]);
         ++index) {
        test_selected_accumulator_decode(&cases[index]);
    }
    test_fixed_pattern_near_misses_are_not_exp();
    test_exhaustive_exp_classification();
    return 0;
}
