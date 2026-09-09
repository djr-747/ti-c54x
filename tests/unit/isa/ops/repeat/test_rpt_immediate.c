/*
 * Module contract
 *
 * Owns: executable COMMON_TI proof that either immediate-RPT form carries
 * its exact decoded operand into architectural RC through the production
 * op and progress leaves with no status payload.
 * Evidence: E-TI-C54X-SPRU172C-S4-RPT-P4-146-4-147 (RC load, no status
 * bits).
 * Does not own: RPT encoding/lookup, repeatability classification (pinned
 * by the eligibility contract), PC flow, interrupt arbitration, timing,
 * buses, or diagnostics. Fields are borrowed; effects are caller-owned.
 */

#include "../../../../../src/isa/decode/operand_field.h"
#include "../../../../../src/isa/decode/operand_layout.h"
#include "../../../../../src/isa/ops/op_effect.h"
#include "../../../../../src/isa/ops/op_status.h"
#include "../../../../../src/isa/ops/repeat/op_rpt_immediate.h"
#include "../../../../../src/sequencer/repeat/repeat_progress.h"
#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/repeat_counter_register.h"
#include "../../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

/* The exact decoded operand must reach architectural RC with no status
 * payload: the produced effect equals {REPEAT_START, operand} bytewise. */
static void
check_operand_reaches_rc(ti_c54x_operand_layout_id id,
                         uint8_t program_word_count,
                         const uint16_t *program_words,
                         uint16_t expected_operand)
{
    const ti_c54x_operand_field_value field =
        extract_operand(id, program_word_count, program_words);
    const uint64_t original_raw = field.raw_value;
    const uint8_t original_width = field.width_bits;
    const ti_c54x_op_effect expected = {
        TI_C54X_OP_EFFECT_REPEAT_START, {{expected_operand}}};
    ti_c54x_op_effect effect;
    ti_c54x_arch_state state;
    ti_c54x_repeat_progress progress = {0};
    uint16_t rc = UINT16_C(0xFFFF);

    memset(&effect, 0xA5, sizeof(effect));
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(&field, &effect) ==
                       TI_C54X_OP_STATUS_OK);
    TI_C54X_TEST_CHECK(memcmp(&effect, &expected, sizeof(effect)) == 0);
    TI_C54X_TEST_CHECK_U64(field.raw_value, original_raw);
    TI_C54X_TEST_CHECK(field.width_bits == original_width);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state,
                                                    &progress) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    TI_C54X_TEST_CHECK_U64(progress.loaded_rc_operand, expected_operand);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rc(&state, &rc) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(rc, expected_operand);
}

static void
test_both_forms_load_rc_exactly(void)
{
    const uint16_t k_min[] = {UINT16_C(0xEC00)};
    const uint16_t k_max[] = {UINT16_C(0xECFF)};
    const uint16_t lk_min[] = {UINT16_C(0xF070), UINT16_C(0x0000)};
    const uint16_t lk_max[] = {UINT16_C(0xF070), UINT16_C(0xFFFF)};

    check_operand_reaches_rc(TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K, 1U,
                             k_min, UINT16_C(0x00));
    check_operand_reaches_rc(TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_K, 1U,
                             k_max, UINT16_C(0xFF));
    check_operand_reaches_rc(TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK, 2U,
                             lk_min, UINT16_C(0x0000));
    check_operand_reaches_rc(TI_C54X_OPERAND_LAYOUT_RPT_IMMEDIATE_LK, 2U,
                             lk_max, UINT16_C(0xFFFF));
}

static void
test_malformed_inputs_rejected_transactionally(void)
{
    const ti_c54x_operand_field_value bad_width = {UINT64_C(0), 7U};
    const ti_c54x_operand_field_value wide_width = {UINT64_C(0), 32U};
    const ti_c54x_operand_field_value over8 = {UINT64_C(0x100), 8U};
    const ti_c54x_operand_field_value over16 = {UINT64_C(0x10000), 16U};
    ti_c54x_op_effect effect;
    unsigned char before[sizeof(effect)];

    memset(&effect, 0xA5, sizeof(effect));
    memcpy(before, &effect, sizeof(before));
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(NULL, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(&bad_width,
                                                          NULL) ==
                       TI_C54X_OP_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(&bad_width,
                                                          &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(&wide_width,
                                                          &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(&over8, &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(ti_c54x_op_rpt_immediate_from_field(&over16,
                                                          &effect) ==
                       TI_C54X_OP_STATUS_INVALID_OPERAND);
    TI_C54X_TEST_CHECK(memcmp(before, &effect, sizeof(effect)) == 0);
}

int
main(void)
{
    test_both_forms_load_rc_exactly();
    test_malformed_inputs_rejected_transactionally();
    puts("rpt_immediate_operation_common: ok");
    return 0;
}
