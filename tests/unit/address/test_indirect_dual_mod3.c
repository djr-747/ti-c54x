/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage for dual-indirect WORD16 MOD3
 * (`*ARx+0%`) planning: BK=0 indexed addition with signed AR0, nonzero-BK
 * circular addressing with the section-5.5.3.4 step rules, same-selector
 * Xmod precedence, and transactional boundary rejection.
 * Evidence: E-TI-C54X-SPRU131G-S5-5-4-T5-8-P5-20-5-21-DUAL-MOD3,
 * E-TI-C54X-SPRU131G-S5-5-4-2-P5-21-5-22-DUAL-MOD3-INDEXED,
 * E-TI-C54X-SPRU131G-S5-5-4-3-P5-21-5-22-DUAL-MOD3-CIRCULAR, and
 * E-TI-C54X-SPRU131G-S5-5-3-4-P5-15-5-16-CIRCULAR-STEP-RULE.
 * Does not own: dual-opcode decoding, CPU state, bus transfers, sequencing,
 * register mutation, timing, or architectural evidence.
 */

#include "address/indirect_dual.h"
#include "address/indirect_dual_mod3.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

static ti_c54x_indirect_dual_result
plan_mod3(uint8_t x_ar,
          uint16_t x_value,
          uint8_t y_ar,
          uint16_t y_value,
          ti_c54x_indirect_dual_mode x_mode,
          ti_c54x_indirect_dual_mode y_mode,
          uint16_t ar0_value,
          uint16_t bk_value)
{
    return ti_c54x_indirect_dual_word16(
        x_ar, x_value, y_ar, y_value, x_mode, y_mode, ar0_value, bk_value,
        TI_C54X_ADDRESS_ACCESS_READ);
}

static void
check_update(const ti_c54x_address_update *update,
             uint8_t ar_index,
             uint16_t before,
             uint16_t after)
{
    TI_C54X_TEST_CHECK(update->target == TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK(update->phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS);
    TI_C54X_TEST_CHECK_U64(update->ar_index, ar_index);
    TI_C54X_TEST_CHECK_U64(update->value_before, before);
    TI_C54X_TEST_CHECK_U64(update->value_after, after);
}

static void
check_no_update(const ti_c54x_address_update *update)
{
    TI_C54X_TEST_CHECK(update->target == TI_C54X_ADDRESS_UPDATE_NONE);
    TI_C54X_TEST_CHECK(update->phase == TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    TI_C54X_TEST_CHECK_U64(update->ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(update->value_before, UINT16_C(0));
    TI_C54X_TEST_CHECK_U64(update->value_after, UINT16_C(0));
}

static void
check_mod3_side(const ti_c54x_address_result *result,
                uint8_t ar_index,
                uint16_t ar_value)
{
    TI_C54X_TEST_CHECK(result->status == TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK(result->effect.operand.width ==
                       TI_C54X_DATA_OPERAND_WIDTH_WORD16);
    TI_C54X_TEST_CHECK_U64(result->effect.operand.first_word, ar_value);
    TI_C54X_TEST_CHECK_U64(result->effect.operand.second_word, UINT16_C(0));
    TI_C54X_TEST_CHECK(result->effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_AR);
    TI_C54X_TEST_CHECK_U64(result->effect.source.value.ar.index, ar_index);
    TI_C54X_TEST_CHECK_U64(result->effect.source.value.ar.value_before,
                           ar_value);
    TI_C54X_TEST_CHECK(result->effect.state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_NONE);
}

static void
check_rejected(const ti_c54x_indirect_dual_result *result,
               ti_c54x_address_status status)
{
    TI_C54X_TEST_CHECK(result->status == status);
    TI_C54X_TEST_CHECK_U64(result->x.effect.operand.width,
                           TI_C54X_DATA_OPERAND_WIDTH_INVALID);
    TI_C54X_TEST_CHECK_U64(result->y.effect.operand.width,
                           TI_C54X_DATA_OPERAND_WIDTH_INVALID);
    TI_C54X_TEST_CHECK(result->x.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_NONE);
    TI_C54X_TEST_CHECK(result->y.effect.source.kind ==
                       TI_C54X_ADDRESS_SOURCE_NONE);
}

/* BK=0 stages plain ARx+AR0 addition; AR0 is a signed step. */
static void
test_indexed_positive_and_negative_steps(void)
{
    ti_c54x_indirect_dual_result plus = plan_mod3(
        UINT8_C(2), UINT16_C(0x1000), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0x0010), UINT16_C(0));
    ti_c54x_indirect_dual_result minus = plan_mod3(
        UINT8_C(2), UINT16_C(0x1000), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0xFFFF), UINT16_C(0));

    TI_C54X_TEST_CHECK(plus.status == TI_C54X_ADDRESS_STATUS_OK);
    check_mod3_side(&plus.x, UINT8_C(2), UINT16_C(0x1000));
    check_update(&plus.x.effect.update, UINT8_C(2), UINT16_C(0x1000),
                 UINT16_C(0x1010));
    TI_C54X_TEST_CHECK(minus.status == TI_C54X_ADDRESS_STATUS_OK);
    check_mod3_side(&minus.x, UINT8_C(2), UINT16_C(0x1000));
    check_update(&minus.x.effect.update, UINT8_C(2), UINT16_C(0x1000),
                 UINT16_C(0x0FFF));
}

/* Indexed overflow/underflow rejects as boundary with no wrap inference. */
static void
test_indexed_boundaries_reject(void)
{
    ti_c54x_indirect_dual_result overflow = plan_mod3(
        UINT8_C(2), UINT16_MAX, UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0x0001), UINT16_C(0));
    ti_c54x_indirect_dual_result underflow = plan_mod3(
        UINT8_C(2), UINT16_C(0), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0xFFFF), UINT16_C(0));

    check_rejected(&overflow, TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_rejected(&underflow, TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
}

/* BK=0 with AR0=0 is the documented no-modification case: no update. */
static void
test_indexed_zero_step_has_no_update(void)
{
    ti_c54x_indirect_dual_result result = plan_mod3(
        UINT8_C(2), UINT16_C(0x1000), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0), UINT16_C(0));

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    check_mod3_side(&result.x, UINT8_C(2), UINT16_C(0x1000));
    check_no_update(&result.x.effect.update);
}

/* Nonzero BK applies the circular algorithm: BK=8 gives EFB size 16. */
static void
test_circular_plain_wrap_and_negative_steps(void)
{
    ti_c54x_indirect_dual_result plain = plan_mod3(
        UINT8_C(2), UINT16_C(0x0103), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0x0002), UINT16_C(8));
    ti_c54x_indirect_dual_result wrap = plan_mod3(
        UINT8_C(2), UINT16_C(0x0107), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0x0002), UINT16_C(8));
    ti_c54x_indirect_dual_result negative = plan_mod3(
        UINT8_C(2), UINT16_C(0x0101), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0xFFFF), UINT16_C(8));
    ti_c54x_indirect_dual_result negative_wrap = plan_mod3(
        UINT8_C(2), UINT16_C(0x0100), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0xFFFF), UINT16_C(8));

    TI_C54X_TEST_CHECK(plain.status == TI_C54X_ADDRESS_STATUS_OK);
    check_update(&plain.x.effect.update, UINT8_C(2), UINT16_C(0x0103),
                 UINT16_C(0x0105));
    TI_C54X_TEST_CHECK(wrap.status == TI_C54X_ADDRESS_STATUS_OK);
    check_update(&wrap.x.effect.update, UINT8_C(2), UINT16_C(0x0107),
                 UINT16_C(0x0101));
    TI_C54X_TEST_CHECK(negative.status == TI_C54X_ADDRESS_STATUS_OK);
    check_update(&negative.x.effect.update, UINT8_C(2), UINT16_C(0x0101),
                 UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(negative_wrap.status == TI_C54X_ADDRESS_STATUS_OK);
    check_update(&negative_wrap.x.effect.update, UINT8_C(2),
                 UINT16_C(0x0100), UINT16_C(0x0107));
}

/* Steps larger than BK and starts outside the buffer reject. */
static void
test_circular_step_and_buffer_rules_reject(void)
{
    ti_c54x_indirect_dual_result big_step = plan_mod3(
        UINT8_C(2), UINT16_C(0x0103), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0x0009), UINT16_C(8));
    ti_c54x_indirect_dual_result big_negative_step = plan_mod3(
        UINT8_C(2), UINT16_C(0x0103), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0xFFF7), UINT16_C(8));
    ti_c54x_indirect_dual_result outside_start = plan_mod3(
        UINT8_C(2), UINT16_C(0x0109), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD0,
        UINT16_C(0x0002), UINT16_C(8));

    check_rejected(&big_step, TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_rejected(&big_negative_step,
                   TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_rejected(&outside_start,
                   TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
}

/* Same-selector alias keeps Xmod precedence with one X-owned update. */
static void
test_same_selector_xmod_precedence(void)
{
    ti_c54x_indirect_dual_result result = plan_mod3(
        UINT8_C(2), UINT16_C(0x0103), UINT8_C(2), UINT16_C(0x0103),
        TI_C54X_INDIRECT_DUAL_MOD3, TI_C54X_INDIRECT_DUAL_MOD1,
        UINT16_C(0x0002), UINT16_C(8));

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    check_mod3_side(&result.x, UINT8_C(2), UINT16_C(0x0103));
    check_update(&result.x.effect.update, UINT8_C(2), UINT16_C(0x0103),
                 UINT16_C(0x0105));
    check_mod3_side(&result.y, UINT8_C(2), UINT16_C(0x0103));
    check_no_update(&result.y.effect.update);
}

/* AR0/BK snapshots are consumed only by MOD3 sides. */
static void
test_non_mod3_plans_ignore_ar0_bk(void)
{
    ti_c54x_indirect_dual_result result = plan_mod3(
        UINT8_C(2), UINT16_C(0x1000), UINT8_C(3), UINT16_C(0x2000),
        TI_C54X_INDIRECT_DUAL_MOD1, TI_C54X_INDIRECT_DUAL_MOD2,
        UINT16_C(0xFFFF), UINT16_C(0xFFFF));

    TI_C54X_TEST_CHECK(result.status == TI_C54X_ADDRESS_STATUS_OK);
    check_update(&result.x.effect.update, UINT8_C(2), UINT16_C(0x1000),
                 UINT16_C(0x0FFF));
    check_update(&result.y.effect.update, UINT8_C(3), UINT16_C(0x2000),
                 UINT16_C(0x2001));
}

/* The update computation is shared with the root validator helper. */
static void
test_mod3_helper_matches_planner(void)
{
    uint16_t after = UINT16_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_indirect_dual_mod3_after(
                           UINT16_C(0x0107), UINT16_C(0x0002),
                           UINT16_C(8), &after) ==
                       TI_C54X_ADDRESS_STATUS_OK);
    TI_C54X_TEST_CHECK_U64(after, UINT16_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_indirect_dual_mod3_after(
                           UINT16_C(0xFFFF), UINT16_C(0x0001),
                           UINT16_C(0), &after) ==
                       TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    TI_C54X_TEST_CHECK(ti_c54x_indirect_dual_mod3_after(
                           UINT16_C(0x0103), UINT16_C(0x0002),
                           UINT16_C(8), NULL) ==
                       TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_indexed_positive_and_negative_steps();
    test_indexed_boundaries_reject();
    test_indexed_zero_step_has_no_update();
    test_circular_plain_wrap_and_negative_steps();
    test_circular_step_and_buffer_rules_reject();
    test_same_selector_xmod_precedence();
    test_non_mod3_plans_ignore_ar0_bk();
    test_mod3_helper_matches_planner();
    puts("indirect_dual_mod3: ok");
    return 0;
}
