/*
 * Module contract
 *
 * Owns: executable proof of immutable catalog-layout resolution, field
 * binding, extraction by operand index, and transactional failure behavior.
 * Does not own: opcode semantics, repeat state, CPU or bus behavior, timing,
 * generated lookup, diagnostics, or test-framework discovery.
 */

#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_field.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
check_value_bytes_unchanged(const unsigned char *before,
                            const ti_c54x_operand_field_value *after)
{
    TI_C54X_TEST_CHECK(memcmp(before, after, sizeof(*after)) == 0);
}

static void
check_resolve_failure(ti_c54x_operand_layout_id id,
                      uint8_t program_word_count,
                      ti_c54x_operand_layout_status expected)
{
    ti_c54x_operand_layout_set_view view;
    unsigned char before[sizeof(view)];

    memset(&view, 0xA5, sizeof(view));
    memcpy(before, &view, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           id, program_word_count, &view) == expected);
    TI_C54X_TEST_CHECK(memcmp(before, &view, sizeof(view)) == 0);
}

static void
test_none_resolution_and_width(void)
{
    ti_c54x_operand_layout_set_view first;
    ti_c54x_operand_layout_set_view second;

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_NONE, 1U, &first) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(first.id == TI_C54X_OPERAND_LAYOUT_NONE);
    TI_C54X_TEST_CHECK(first.program_word_count == 1U);
    TI_C54X_TEST_CHECK(first.descriptor->operand_count == 0U);
    TI_C54X_TEST_CHECK(first.descriptor->operands == NULL);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_NONE, 2U, &second) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(second.descriptor == first.descriptor);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_NONE, 1U, &second) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(first.descriptor == second.descriptor);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &first, &(const ti_c54x_program_words_view){NULL, 0U},
                           0U, &(ti_c54x_operand_field_value){0U, 0U}) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OPERAND_INDEX_OUT_OF_RANGE);
}

static void
test_rpt_k_resolution_and_extraction(void)
{
    const uint16_t words[] = {UINT16_C(0xEC00), UINT16_C(0xFFFF)};
    const ti_c54x_program_words_view word_view = {words, 1U};
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value value = {0U, 0U};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K, 1U, &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == 1U);
    TI_C54X_TEST_CHECK(view.descriptor->operands != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 0U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(0));
    TI_C54X_TEST_CHECK(value.width_bits == 8U);

    {
        const uint16_t maximum_words[] = {UINT16_C(0xECFF)};
        const ti_c54x_program_words_view maximum_view = {maximum_words, 1U};
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &maximum_view, 0U, &value) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(0xFF));
    }
}

static void
test_rpt_lk_resolution_and_extraction(void)
{
    const uint16_t words[] = {UINT16_C(0xF070), UINT16_C(0xBEEF)};
    const ti_c54x_program_words_view word_view = {words, 2U};
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value value = {0U, 0U};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK, 2U,
                           &view) == TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 0U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(0xBEEF));
    TI_C54X_TEST_CHECK(value.width_bits == 16U);

    {
        const uint16_t minimum_words[] = {
            UINT16_C(0xF070), UINT16_C(0x0000)};
        const ti_c54x_program_words_view minimum_view = {minimum_words, 2U};
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &minimum_view, 0U, &value) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(0x0000));
    }
    {
        const uint16_t maximum_words[] = {
            UINT16_C(0xF070), UINT16_C(0xFFFF)};
        const ti_c54x_program_words_view maximum_view = {maximum_words, 2U};
        TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                               &view, &maximum_view, 0U, &value) ==
                           TI_C54X_OPERAND_LAYOUT_STATUS_OK);
        TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(0xFFFF));
    }
}

static void
test_resolution_ids_and_widths(void)
{
    /* Production RPT rows landed with the P3-S2 activation slice;
     * their first words are pinned by the rpt_k/rpt_lk lanes. */
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_NONE, 1U, NULL) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT);
    check_resolve_failure(TI_C54X_OPERAND_LAYOUT_UNKNOWN, 1U,
                          TI_C54X_OPERAND_LAYOUT_STATUS_UNKNOWN_ID);
    check_resolve_failure((ti_c54x_operand_layout_id)-1, 1U,
                          TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ID);
    check_resolve_failure(TI_C54X_OPERAND_LAYOUT_COUNT, 1U,
                          TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ID);
    check_resolve_failure(TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K, 2U,
                          TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH);
    check_resolve_failure(TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK, 1U,
                          TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH);
    /* The bundle capacity admits three-word instructions, so the
     * out-of-range count probe sits one past the new capacity. */
    check_resolve_failure(TI_C54X_OPERAND_LAYOUT_NONE, 4U,
                          TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_INVALID);
}

static void
test_nulls_indexes_and_output_transactionality(void)
{
    const uint16_t words[] = {UINT16_C(0xEC12), UINT16_C(0x1234)};
    const ti_c54x_program_words_view word_view = {words, 1U};
    const ti_c54x_program_words_view mismatched_word_view = {words, 2U};
    const ti_c54x_program_words_view invalid_word_view = {NULL, 1U};
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value value;
    unsigned char before[sizeof(value)];

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K, 1U, &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    memset(&value, 0xA5, sizeof(value));
    memcpy(before, &value, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           NULL, &word_view, 0U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT);
    check_value_bytes_unchanged(before, &value);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, NULL, 0U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT);
    check_value_bytes_unchanged(before, &value);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 1U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OPERAND_INDEX_OUT_OF_RANGE);
    check_value_bytes_unchanged(before, &value);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &mismatched_word_view, 0U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_MISMATCH);
    check_value_bytes_unchanged(before, &value);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &invalid_word_view, 0U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT);
    check_value_bytes_unchanged(before, &value);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 0U, NULL) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT);
}

static void
test_local_multi_operand_descriptor_validation(void)
{
    const ti_c54x_operand_field_piece first_piece = {0U, 0U, 8U, 0U};
    const ti_c54x_operand_field_piece second_piece = {1U, 0U, 8U, 0U};
    const ti_c54x_operand_field_layout fields[] = {
        {&first_piece, 1U, 8U}, {&second_piece, 1U, 8U}};
    const ti_c54x_operand_field_piece overlap_pieces[] = {
        {0U, 0U, 4U, 0U}, {0U, 4U, 4U, 0U}};
    const ti_c54x_operand_field_layout overlap_field = {
        overlap_pieces, 2U, 8U};
    const ti_c54x_operand_layout_set local = {fields, 2U};
    const ti_c54x_operand_layout_set overlap = {&overlap_field, 1U};
    ti_c54x_operand_layout_set_view forged = {
        TI_C54X_OPERAND_LAYOUT_NONE, 2U, &local};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_validate_descriptor(
                           &local, 2U) == TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_validate_descriptor(
                           &local, 1U) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_FIELD_SOURCE_WORD_INDEX_OUT_OF_RANGE);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_validate_descriptor(
                           &overlap, 1U) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_FIELD_LAYOUT_INVALID);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_validate_descriptor(
                           &(const ti_c54x_operand_layout_set){NULL, 1U}, 1U) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_SET);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_validate_descriptor(
                           NULL, 1U) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_validate_descriptor(
                           &local, 0U) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_PROGRAM_WORD_COUNT_INVALID);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &forged,
                           &(const ti_c54x_program_words_view){
                               (const uint16_t[]){UINT16_C(0), UINT16_C(0)},
                               2U},
                           0U, &(ti_c54x_operand_field_value){0U, 0U}) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_INVALID_VIEW);
}

static void
test_select_dual_xmod_xar_cond4_resolution_and_extraction(void)
{
    /* SRCCD-family shape: S=1, XMOD=1, XAR=1, COND=0xB (BLT). */
    const uint16_t words[] = {UINT16_C(0x9D5B)};
    const ti_c54x_program_words_view word_view = {words, 1U};
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value value = {0U, 0U};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_SELECT_DUAL_XMOD_XAR_COND4,
                           1U, &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == 4U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 0U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(1));
    TI_C54X_TEST_CHECK(value.width_bits == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 1U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(1));
    TI_C54X_TEST_CHECK(value.width_bits == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 2U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(1));
    TI_C54X_TEST_CHECK(value.width_bits == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 3U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(0xB));
    TI_C54X_TEST_CHECK(value.width_bits == 4U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 4U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OPERAND_INDEX_OUT_OF_RANGE);
}

static void
test_dual_xmod_xar_bitc_resolution_and_extraction(void)
{
    /* BIT shape: XMOD=1, XAR=2, BITC=0x3. */
    const uint16_t words[] = {UINT16_C(0x9663)};
    const ti_c54x_program_words_view word_view = {words, 1U};
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value value = {0U, 0U};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           TI_C54X_OPERAND_LAYOUT_DUAL_XMOD_XAR_BITC,
                           1U, &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(view.descriptor->operand_count == 3U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 0U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(1));
    TI_C54X_TEST_CHECK(value.width_bits == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 1U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(2));
    TI_C54X_TEST_CHECK(value.width_bits == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 2U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(value.raw_value, UINT64_C(0x3));
    TI_C54X_TEST_CHECK(value.width_bits == 4U);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &word_view, 3U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OPERAND_INDEX_OUT_OF_RANGE);
}

int
main(void)
{
    test_none_resolution_and_width();
    test_rpt_k_resolution_and_extraction();
    test_rpt_lk_resolution_and_extraction();
    test_resolution_ids_and_widths();
    test_nulls_indexes_and_output_transactionality();
    test_local_multi_operand_descriptor_validation();
    test_select_dual_xmod_xar_cond4_resolution_and_extraction();
    test_dual_xmod_xar_bitc_resolution_and_extraction();
    puts("operand_layout_binding: ok");
    return 0;
}
