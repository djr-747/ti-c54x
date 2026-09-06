/*
 * Module contract
 *
 * Owns: exhaustive first-word classification for the planned SFTL masked
 * catalog row and extraction of its generic S, D, and raw five-bit shift
 * operands. Signed shift interpretation is deliberately test-side only.
 * Does not own: shift semantics, accumulator mutation, CPU/state integration,
 * sequencing, timing, or catalog generation.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct sftl_case {
    uint16_t word;
    uint8_t source;
    uint8_t destination;
    uint8_t shift_raw;
    int8_t shift_signed;
} sftl_case;

static const sftl_case cases[] = {
    {UINT16_C(0xF0E0), UINT8_C(0), UINT8_C(0), UINT8_C(0), INT8_C(0)},
    {UINT16_C(0xF0E1), UINT8_C(0), UINT8_C(0), UINT8_C(1), INT8_C(1)},
    {UINT16_C(0xF0EF), UINT8_C(0), UINT8_C(0), UINT8_C(15), INT8_C(15)},
    {UINT16_C(0xF0F0), UINT8_C(0), UINT8_C(0), UINT8_C(16), INT8_C(-16)},
    {UINT16_C(0xF0FF), UINT8_C(0), UINT8_C(0), UINT8_C(31), INT8_C(-1)},
    {UINT16_C(0xF1E0), UINT8_C(0), UINT8_C(1), UINT8_C(0), INT8_C(0)},
    {UINT16_C(0xF2E0), UINT8_C(1), UINT8_C(0), UINT8_C(0), INT8_C(0)},
    {UINT16_C(0xF3FF), UINT8_C(1), UINT8_C(1), UINT8_C(31), INT8_C(-1)},
};

static int8_t
sign_extend_shift5(uint8_t raw)
{
    const uint8_t value = (uint8_t)(raw & UINT8_C(0x1F));
    return (int8_t)((value & UINT8_C(0x10)) != UINT8_C(0)
                        ? (int)value - 32
                        : (int)value);
}

static void
check_operands(const sftl_case *test_case)
{
    const ti_c54x_decode_result first =
        ti_c54x_decode_first_word(test_case->word);
    const ti_c54x_instruction_word_bundle bundle = {
        .program_words = {test_case->word, UINT16_C(0)},
        .program_word_count = UINT8_C(1)};
    ti_c54x_decoded_instruction instruction;
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value source = {0U, 0U};
    ti_c54x_operand_field_value destination = {0U, 0U};
    ti_c54x_operand_field_value shift = {0U, 0U};
    const ti_c54x_program_words_view words = {
        bundle.program_words, bundle.program_word_count};

    TI_C54X_TEST_CHECK(first.status == TI_C54X_DECODE_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPCODE_ID_SFTL_ACC);
    TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(
                           &first, &bundle, &instruction) ==
                       TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);
    TI_C54X_TEST_CHECK(instruction.operand_layout ==
                       TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           instruction.operand_layout, UINT8_C(1), &view) ==
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
    TI_C54X_TEST_CHECK(source.raw_value == test_case->source);
    TI_C54X_TEST_CHECK(destination.raw_value == test_case->destination);
    TI_C54X_TEST_CHECK(shift.raw_value == test_case->shift_raw);
    TI_C54X_TEST_CHECK(sign_extend_shift5((uint8_t)shift.raw_value) ==
                       test_case->shift_signed);
}

static void
test_exhaustive_classification(void)
{
    unsigned valid = 0U;

    for (uint32_t raw = 0U; raw <= UINT16_MAX; ++raw) {
        const uint16_t word = (uint16_t)raw;
        const ti_c54x_decode_result decoded =
            ti_c54x_decode_first_word(word);
        const bool matches =
            (word & UINT16_C(0xFCE0)) == UINT16_C(0xF0E0);

        TI_C54X_TEST_CHECK((decoded.id == TI_C54X_OPCODE_ID_SFTL_ACC) ==
                           matches);
        if (matches) {
            ++valid;
            TI_C54X_TEST_CHECK(decoded.status == TI_C54X_DECODE_STATUS_OK);
        }
    }
    TI_C54X_TEST_CHECK(valid == 128U);
}

static void
test_fixed_pattern_near_misses_and_unrelated_rows(void)
{
    static const uint16_t near_misses[] = {
        UINT16_C(0xF4E0), UINT16_C(0xF8E0), UINT16_C(0xE0E0),
        UINT16_C(0xF0C0), UINT16_C(0xF060)};
    static const uint16_t unrelated[] = {
        UINT16_C(0xF495), UINT16_C(0xEA00), UINT16_C(0xF486),
        UINT16_C(0xF493), UINT16_C(0xF491)};

    for (size_t i = 0U; i < sizeof(near_misses) / sizeof(near_misses[0]);
         ++i) {
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(near_misses[i]).id !=
                           TI_C54X_OPCODE_ID_SFTL_ACC);
    }
    for (size_t i = 0U; i < sizeof(unrelated) / sizeof(unrelated[0]); ++i) {
        TI_C54X_TEST_CHECK(ti_c54x_decode_first_word(unrelated[i]).id !=
                           TI_C54X_OPCODE_ID_SFTL_ACC);
    }
}

int
main(void)
{
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        check_operands(&cases[i]);
    }
    test_exhaustive_classification();
    test_fixed_pattern_near_misses_and_unrelated_rows();
    return 0;
}
