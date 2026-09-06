/*
 * Module contract
 *
 * Owns: executable proof of the pure operand-field extractor's valid
 * assembly, complete validation matrix, and transactional output behavior.
 * Does not own: opcode meanings, bus/fetch behavior, CPU state, sequencing,
 * timing, diagnostics, or external test-framework discovery.
 */

#include "../../../../src/isa/decode/operand_field.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
check_unchanged(const ti_c54x_operand_field_value *before,
                const ti_c54x_operand_field_value *after)
{
    TI_C54X_TEST_CHECK(memcmp(before, after, sizeof(*before)) == 0);
}

static ti_c54x_operand_field_status
extract(const uint16_t *words,
        uint8_t word_count,
        const ti_c54x_operand_field_piece *pieces,
        uint8_t piece_count,
        uint8_t value_width,
        ti_c54x_operand_field_value *value)
{
    const ti_c54x_program_words_view view = {words, word_count};
    const ti_c54x_operand_field_layout layout = {
        pieces, piece_count, value_width};
    return ti_c54x_operand_field_extract(&view, &layout, value);
}

static void
test_every_valid_one_word_slice(void)
{
    const uint16_t words[] = {UINT16_C(0xA55A)};
    uint8_t source_lsb;

    for (source_lsb = 0U; source_lsb < 16U; ++source_lsb) {
        uint8_t width;
        for (width = 1U; width <= (uint8_t)(16U - source_lsb); ++width) {
            const ti_c54x_operand_field_piece piece = {
                0U, source_lsb, width, 0U};
            ti_c54x_operand_field_value value = {0U, 0U};
            const uint32_t mask = (UINT32_C(1) << width) - UINT32_C(1);
            const uint64_t expected =
                (uint64_t)((words[0] >> source_lsb) & mask);

            TI_C54X_TEST_CHECK(ti_c54x_operand_field_extract(
                                   &(const ti_c54x_program_words_view){
                                       words, 1U},
                                   &(const ti_c54x_operand_field_layout){
                                       &piece, 1U, width},
                                   &value) ==
                               TI_C54X_OPERAND_FIELD_STATUS_OK);
            TI_C54X_TEST_CHECK_U64(value.raw_value, expected);
            TI_C54X_TEST_CHECK(value.width_bits == width);
        }
    }
}

static void
test_extension_split_and_piece_order(void)
{
    const uint16_t words[] = {UINT16_C(0x0A0B), UINT16_C(0x00CD)};
    const ti_c54x_operand_field_piece pieces[] = {
        {1U, 0U, 8U, 4U},
        {0U, 0U, 4U, 0U},
        {0U, 8U, 4U, 12U}};
    const ti_c54x_operand_field_piece reversed[] = {
        {0U, 8U, 4U, 12U},
        {0U, 0U, 4U, 0U},
        {1U, 0U, 8U, 4U}};
    ti_c54x_operand_field_value value = {0U, 0U};
    ti_c54x_operand_field_value reverse_value = {0U, 0U};

    TI_C54X_TEST_CHECK(extract(words, 2U, pieces, 3U, 16U, &value) ==
                       TI_C54X_OPERAND_FIELD_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(0xACDB));
    TI_C54X_TEST_CHECK(value.width_bits == 16U);
    TI_C54X_TEST_CHECK(extract(words, 2U, reversed, 3U, 16U,
                               &reverse_value) ==
                       TI_C54X_OPERAND_FIELD_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(reverse_value.raw_value, value.raw_value);
    TI_C54X_TEST_CHECK(reverse_value.width_bits == value.width_bits);
}

static void
test_full_64_bit_four_word_assembly(void)
{
    const uint16_t words[] = {
        UINT16_C(0x0123), UINT16_C(0x4567), UINT16_C(0x89AB), UINT16_C(0xCDEF)};
    const ti_c54x_operand_field_piece pieces[] = {
        {3U, 0U, 16U, 48U},
        {1U, 0U, 16U, 16U},
        {0U, 0U, 16U, 0U},
        {2U, 0U, 16U, 32U}};
    ti_c54x_operand_field_value value = {0U, 0U};

    TI_C54X_TEST_CHECK(extract(words, 4U, pieces, 4U, 64U, &value) ==
                       TI_C54X_OPERAND_FIELD_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(0xCDEF89AB45670123));
    TI_C54X_TEST_CHECK(value.width_bits == 64U);
}

static void
test_input_words_are_borrowed_not_retained(void)
{
    uint16_t words[] = {UINT16_C(0x0001)};
    const ti_c54x_operand_field_piece piece = {0U, 0U, 1U, 0U};
    ti_c54x_operand_field_value value = {0U, 0U};

    TI_C54X_TEST_CHECK(extract(words, 1U, &piece, 1U, 1U, &value) ==
                       TI_C54X_OPERAND_FIELD_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(1));
    words[0] = UINT16_C(0x0000);
    TI_C54X_TEST_CHECK(extract(words, 1U, &piece, 1U, 1U, &value) ==
                       TI_C54X_OPERAND_FIELD_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(0));
}

static void
test_invalid_arguments_and_views_are_transactional(void)
{
    const uint16_t words[] = {UINT16_C(0xFFFF)};
    const ti_c54x_operand_field_piece piece = {0U, 0U, 1U, 0U};
    const ti_c54x_program_words_view view = {words, 1U};
    const ti_c54x_operand_field_layout layout = {&piece, 1U, 1U};
    const ti_c54x_operand_field_value before = {
        UINT64_C(0xA5A5A5A5A5A5A5A5), 63U};
    ti_c54x_operand_field_value value = before;

    TI_C54X_TEST_CHECK(ti_c54x_operand_field_extract(NULL, &layout, &value) ==
                       TI_C54X_OPERAND_FIELD_STATUS_INVALID_ARGUMENT);
    check_unchanged(&before, &value);
    value = before;
    TI_C54X_TEST_CHECK(ti_c54x_operand_field_extract(&view, NULL, &value) ==
                       TI_C54X_OPERAND_FIELD_STATUS_INVALID_ARGUMENT);
    check_unchanged(&before, &value);
    value = before;
    TI_C54X_TEST_CHECK(ti_c54x_operand_field_extract(&view, &layout, NULL) ==
                       TI_C54X_OPERAND_FIELD_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_operand_field_extract(
                           &(const ti_c54x_program_words_view){NULL, 1U},
                           &layout, &value) ==
                       TI_C54X_OPERAND_FIELD_STATUS_INVALID_WORD_VIEW);
    check_unchanged(&before, &value);
    value = before;
    TI_C54X_TEST_CHECK(ti_c54x_operand_field_extract(
                           &(const ti_c54x_program_words_view){words, 0U},
                           &layout, &value) ==
                       TI_C54X_OPERAND_FIELD_STATUS_INVALID_WORD_VIEW);
    check_unchanged(&before, &value);
}

static void
check_invalid_layout(const uint16_t *words,
                     uint8_t word_count,
                     const ti_c54x_operand_field_piece *pieces,
                     uint8_t piece_count,
                     uint8_t value_width)
{
    const ti_c54x_operand_field_value before = {
        UINT64_C(0x1122334455667788), 37U};
    ti_c54x_operand_field_value value = before;

    TI_C54X_TEST_CHECK(extract(words, word_count, pieces, piece_count,
                               value_width, &value) ==
                       TI_C54X_OPERAND_FIELD_STATUS_INVALID_LAYOUT);
    check_unchanged(&before, &value);
}

static void
test_invalid_layouts_and_indexes_are_transactional(void)
{
    const uint16_t words[] = {UINT16_C(0xFFFF), UINT16_C(0xFFFF)};
    const ti_c54x_operand_field_piece valid = {0U, 0U, 1U, 0U};
    const ti_c54x_operand_field_piece zero_width = {0U, 0U, 0U, 0U};
    const ti_c54x_operand_field_piece oversize_width = {0U, 0U, 17U, 0U};
    const ti_c54x_operand_field_piece source_lsb_overflow = {0U, 16U, 1U, 0U};
    const ti_c54x_operand_field_piece source_span_overflow = {0U, 15U, 2U, 0U};
    const ti_c54x_operand_field_piece destination_lsb_overflow = {0U, 0U, 1U,
                                                                   64U};
    const ti_c54x_operand_field_piece destination_span_overflow = {0U, 0U, 2U,
                                                                    63U};
    const ti_c54x_operand_field_piece overlap[] = {
        {0U, 0U, 2U, 0U}, {0U, 2U, 2U, 1U}};
    const ti_c54x_operand_field_piece gap[] = {
        {0U, 0U, 1U, 0U}, {0U, 2U, 1U, 2U}};
    const ti_c54x_operand_field_piece source_overlap[] = {
        {0U, 0U, 2U, 0U}, {0U, 1U, 2U, 2U}};
    const ti_c54x_operand_field_piece out_of_range = {2U, 0U, 1U, 0U};

    check_invalid_layout(words, 2U, NULL, 1U, 1U);
    check_invalid_layout(words, 2U, &valid, 0U, 1U);
    check_invalid_layout(words, 2U, &valid, 1U, 0U);
    check_invalid_layout(words, 2U, &valid, 1U, 65U);
    check_invalid_layout(words, 2U, &zero_width, 1U, 1U);
    check_invalid_layout(words, 2U, &oversize_width, 1U, 17U);
    check_invalid_layout(words, 2U, &source_lsb_overflow, 1U, 1U);
    check_invalid_layout(words, 2U, &source_span_overflow, 1U, 2U);
    check_invalid_layout(words, 2U, &destination_lsb_overflow, 1U, 1U);
    check_invalid_layout(words, 2U, &destination_span_overflow, 1U, 64U);
    check_invalid_layout(words, 2U, overlap, 2U, 3U);
    check_invalid_layout(words, 2U, gap, 2U, 3U);
    check_invalid_layout(words, 2U, source_overlap, 2U, 4U);

    {
        const ti_c54x_operand_field_value before = {
            UINT64_C(0x1122334455667788), 37U};
        ti_c54x_operand_field_value value = before;

        TI_C54X_TEST_CHECK(extract(words, 2U, &out_of_range, 1U, 1U, &value) ==
                           TI_C54X_OPERAND_FIELD_STATUS_SOURCE_WORD_INDEX_OUT_OF_RANGE);
        check_unchanged(&before, &value);
    }
}

static void
test_descriptor_validation_seam_matches_extraction_rules(void)
{
    const ti_c54x_operand_field_piece pieces[] = {
        {0U, 0U, 8U, 0U}, {1U, 0U, 8U, 8U}};
    const ti_c54x_operand_field_layout layout = {pieces, 2U, 16U};
    const ti_c54x_operand_field_piece bad_source = {2U, 0U, 16U, 0U};
    const ti_c54x_operand_field_layout bad_layout = {&bad_source, 1U, 16U};

    TI_C54X_TEST_CHECK(ti_c54x_operand_field_validate_layout(&layout, 2U) ==
                       TI_C54X_OPERAND_FIELD_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_operand_field_validate_layout(&layout, 1U) ==
                       TI_C54X_OPERAND_FIELD_STATUS_SOURCE_WORD_INDEX_OUT_OF_RANGE);
    TI_C54X_TEST_CHECK(ti_c54x_operand_field_validate_layout(&bad_layout, 2U) ==
                       TI_C54X_OPERAND_FIELD_STATUS_SOURCE_WORD_INDEX_OUT_OF_RANGE);
    TI_C54X_TEST_CHECK(ti_c54x_operand_field_validate_layout(NULL, 1U) ==
                       TI_C54X_OPERAND_FIELD_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_operand_field_validate_layout(&layout, 0U) ==
                       TI_C54X_OPERAND_FIELD_STATUS_INVALID_WORD_VIEW);
}

int
main(void)
{
    test_every_valid_one_word_slice();
    test_extension_split_and_piece_order();
    test_full_64_bit_four_word_assembly();
    test_input_words_are_borrowed_not_retained();
    test_invalid_arguments_and_views_are_transactional();
    test_invalid_layouts_and_indexes_are_transactional();
    test_descriptor_validation_seam_matches_extraction_rules();
    puts("operand_field: ok");
    return 0;
}
