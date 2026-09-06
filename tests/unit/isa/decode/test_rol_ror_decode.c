/*
 * Module contract
 *
 * Owns: exhaustive first-word classification for the ROL/ROR masked rows and
 * extraction of their one-bit accumulator source operand from bit 8.
 * Does not own: rotate semantics, CPU/state integration, timing, or catalog
 * generation.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct rotate_case {
    uint16_t word;
    ti_c54x_opcode_id id;
    uint8_t source;
} rotate_case;

static const rotate_case cases[] = {
    {UINT16_C(0xF491), TI_C54X_OPCODE_ID_ROL_ACC, UINT8_C(0)},
    {UINT16_C(0xF591), TI_C54X_OPCODE_ID_ROL_ACC, UINT8_C(1)},
    {UINT16_C(0xF490), TI_C54X_OPCODE_ID_ROR_ACC, UINT8_C(0)},
    {UINT16_C(0xF590), TI_C54X_OPCODE_ID_ROR_ACC, UINT8_C(1)},
};

static void
check_source_operand(const rotate_case *test_case)
{
    const ti_c54x_opcode_metadata *metadata =
        ti_c54x_opcode_metadata_for_id(test_case->id);
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(test_case->word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {test_case->word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value field = {0U, 0U};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};

    TI_C54X_TEST_CHECK(metadata != NULL);
    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == test_case->id);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE,
                           UINT8_C(1), &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &words, UINT8_C(0), &field) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(field.width_bits == UINT8_C(1));
    TI_C54X_TEST_CHECK(field.raw_value == test_case->source);
}

static void
test_exhaustive_classification(void)
{
    unsigned valid = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);
        const bool exact = word == UINT16_C(0xF491) ||
                           word == UINT16_C(0xF591) ||
                           word == UINT16_C(0xF490) ||
                           word == UINT16_C(0xF590);
        const bool rotate = decoded.id == TI_C54X_OPCODE_ID_ROL_ACC ||
                            decoded.id == TI_C54X_OPCODE_ID_ROR_ACC;

        TI_C54X_TEST_CHECK(rotate == exact);
        if (exact) {
            ++valid;
            TI_C54X_TEST_CHECK(decoded.status == TI_C54X_DECODE_STATUS_OK);
        }
    }
    TI_C54X_TEST_CHECK(valid == 4U);
}

static void
test_fixed_pattern_near_misses_and_unrelated_rows(void)
{
    static const uint16_t near_misses[] = {
        UINT16_C(0xF492), UINT16_C(0xF4B1), UINT16_C(0xF691),
        UINT16_C(0x7491), UINT16_C(0xF4D1)};
    static const uint16_t unrelated[] = {
        UINT16_C(0xF495), UINT16_C(0xEA00), UINT16_C(0xF486),
        UINT16_C(0xF487), UINT16_C(0xF493)};

    for (size_t i = 0U; i < sizeof(near_misses) / sizeof(near_misses[0]);
         ++i) {
        const ti_c54x_opcode_id id =
            ti_c54x_decode_first_word(near_misses[i]).id;
        TI_C54X_TEST_CHECK(id != TI_C54X_OPCODE_ID_ROL_ACC);
        TI_C54X_TEST_CHECK(id != TI_C54X_OPCODE_ID_ROR_ACC);
    }
    for (size_t i = 0U; i < sizeof(unrelated) / sizeof(unrelated[0]); ++i) {
        const ti_c54x_opcode_id id =
            ti_c54x_decode_first_word(unrelated[i]).id;
        TI_C54X_TEST_CHECK(id != TI_C54X_OPCODE_ID_ROL_ACC);
        TI_C54X_TEST_CHECK(id != TI_C54X_OPCODE_ID_ROR_ACC);
    }
}

int
main(void)
{
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        check_source_operand(&cases[i]);
    }
    test_exhaustive_classification();
    test_fixed_pattern_near_misses_and_unrelated_rows();
    return 0;
}
