/*
 * Module contract
 *
 * Owns: executable proof that extracted K/lk fields become value-owned
 * repeat-start effects and malformed inputs fail transactionally.
 * Does not own: opcode encoding/lookup, field-extractor implementation,
 * repeat state, CPU integration, timing, buses, or diagnostics.
 */

#include "../../../../../src/isa/decode/operand_layout.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/repeat/op_rpt_immediate.h"
#include "../../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
check_effect_unchanged(const unsigned char *before,
                       const ti_c54x_op_effect *effect)
{
    TI_C54X_TEST_CHECK(memcmp(before, effect, sizeof(*effect)) == 0);
}

static ti_c54x_operand_field_value
extract_operand(ti_c54x_operand_layout_id id,
                uint8_t program_word_count,
                const uint16_t *program_words)
{
    ti_c54x_operand_layout_set_view view;
    ti_c54x_operand_field_value value = {0U, 0U};
    const ti_c54x_program_words_view input = {
        program_words, program_word_count};

    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_resolve(
                           id, program_word_count, &view) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_operand_layout_set_extract_operand(
                           &view, &input, 0U, &value) ==
                       TI_C54X_OPERAND_LAYOUT_STATUS_OK);
    return value;
}

static void
check_success(const ti_c54x_operand_field_value *field, uint16_t expected)
{
    const uint64_t original_raw_value = field->raw_value;
    const uint8_t original_width_bits = field->width_bits;
    ti_c54x_op_effect effect;

    memset(&effect, 0xA5, sizeof(effect));
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(field, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_REPEAT_START);
    TI_C54X_TEST_CHECK(effect.payload.repeat_start.operand == expected);
    TI_C54X_TEST_CHECK_U64(field->raw_value, original_raw_value);
    TI_C54X_TEST_CHECK(field->width_bits == original_width_bits);
}

static void
test_layout_composed_vectors(void)
{
    const uint16_t k0[] = {UINT16_C(0xEC00)};
    const uint16_t k20[] = {UINT16_C(0xEC20)};
    const uint16_t kff[] = {UINT16_C(0xECFF)};
    const uint16_t l0[] = {UINT16_C(0xF070), UINT16_C(0)};
    const uint16_t lb[] = {UINT16_C(0xF070), UINT16_C(0xBEEF)};
    const uint16_t lff[] = {UINT16_C(0xF070), UINT16_C(0xFFFF)};
    ti_c54x_operand_field_value field;

    field = extract_operand(
        TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K, 1U, k0);
    check_success(&field, UINT16_C(0));
    field = extract_operand(
        TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K, 1U, k20);
    check_success(&field, UINT16_C(0x20));
    field = extract_operand(
        TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K, 1U, kff);
    check_success(&field, UINT16_C(0xff));
    field = extract_operand(
        TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK, 2U, l0);
    check_success(&field, UINT16_C(0));
    field = extract_operand(
        TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK, 2U, lb);
    check_success(&field, UINT16_C(0xbeef));
    field = extract_operand(
        TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK, 2U, lff);
    check_success(&field, UINT16_C(0xffff));
}

static void
test_effect_retains_no_operand_pointer(void)
{
    ti_c54x_operand_field_value field = {UINT64_C(0x34), UINT8_C(8)};
    ti_c54x_op_effect effect;

    memset(&effect, 0, sizeof(effect));
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(&field, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    field.raw_value = UINT64_C(0xff);
    field.width_bits = UINT8_C(16);
    TI_C54X_TEST_CHECK(effect.kind == TI_C54X_OP_EFFECT_REPEAT_START);
    TI_C54X_TEST_CHECK(effect.payload.repeat_start.operand == UINT16_C(0x34));
}

static void
test_failures_are_typed_and_transactional(void)
{
    const uint8_t widths[] = {0U, 7U, 9U, 15U, 17U, 64U};
    size_t i;

    for (i = 0U; i < sizeof(widths) / sizeof(widths[0]); ++i) {
        const ti_c54x_operand_field_value field = {UINT64_C(1), widths[i]};
        ti_c54x_op_effect effect;
        unsigned char before[sizeof(effect)];
        memset(&effect, 0x31, sizeof(effect));
        memcpy(before, &effect, sizeof(before));
        TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(&field, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_OPERAND);
        check_effect_unchanged(before, &effect);
    }
    {
        const ti_c54x_operand_field_value bad[] = {
            {UINT64_C(0x100), UINT8_C(8)},
            {UINT64_C(0x10000), UINT8_C(16)}};
        for (i = 0U; i < sizeof(bad) / sizeof(bad[0]); ++i) {
            ti_c54x_op_effect effect;
            unsigned char before[sizeof(effect)];
            memset(&effect, 0x42, sizeof(effect));
            memcpy(before, &effect, sizeof(before));
            TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(
                                   &bad[i], &effect) ==
                               TI_C54X_OP_STATUS_INVALID_OPERAND);
            check_effect_unchanged(before, &effect);
        }
    }
    {
        const ti_c54x_operand_field_value field = {UINT64_C(0), UINT8_C(8)};
        ti_c54x_op_effect effect;
        unsigned char before[sizeof(effect)];
        memset(&effect, 0x53, sizeof(effect));
        memcpy(before, &effect, sizeof(before));
        TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(NULL, &effect) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        check_effect_unchanged(before, &effect);
        TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(&field, NULL) ==
                           TI_C54X_OP_STATUS_INVALID_ARGUMENT);
        check_effect_unchanged(before, &effect);
    }
}

int
main(void)
{
    test_layout_composed_vectors();
    test_effect_retains_no_operand_pointer();
    test_failures_are_typed_and_transactional();
    puts("rpt_immediate_effect: ok");
    return 0;
}
