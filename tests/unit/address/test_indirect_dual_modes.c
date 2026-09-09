/*
 * Module contract
 *
 * Owns: executable COMMON_TI coverage for dual-indirect WORD16 MOD0/MOD1/
 * MOD2 planning, including independent selectors, transactional boundaries,
 * and same-selector single-update aliasing.
 * Does not own: dual-opcode decoding, CPU state, bus transfers, sequencing,
 * register mutation, timing, or architectural evidence.
 */

#include "address/indirect_dual.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>

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
check_operand(const ti_c54x_address_result *result,
              uint8_t ar_index,
              uint16_t ar_value,
              ti_c54x_address_update_target expected_update,
              uint16_t expected_after)
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
    if (expected_update == TI_C54X_ADDRESS_UPDATE_NONE) {
        check_no_update(&result->effect.update);
    } else {
        check_update(&result->effect.update, ar_index, ar_value,
                     expected_after);
    }
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
    check_no_update(&result->x.effect.update);
    check_no_update(&result->y.effect.update);
}

static void
test_distinct_selectors_plan_independently(void)
{
    static const ti_c54x_indirect_dual_mode modes[] = {
        TI_C54X_INDIRECT_DUAL_MOD0,
        TI_C54X_INDIRECT_DUAL_MOD1,
        TI_C54X_INDIRECT_DUAL_MOD2};
    static const uint16_t x_value = UINT16_C(0x1234);
    static const uint16_t y_value = UINT16_C(0x5678);

    for (size_t x_mode_index = 0; x_mode_index < 3; ++x_mode_index) {
        for (size_t y_mode_index = 0; y_mode_index < 3; ++y_mode_index) {
            const ti_c54x_indirect_dual_mode x_mode = modes[x_mode_index];
            const ti_c54x_indirect_dual_mode y_mode = modes[y_mode_index];
            const ti_c54x_indirect_dual_result result =
                ti_c54x_indirect_dual_word16(
                    UINT8_C(2), x_value, UINT8_C(3), y_value, x_mode,
                    y_mode, UINT16_C(0), UINT16_C(0),
                    TI_C54X_ADDRESS_ACCESS_READ);
            const ti_c54x_address_update_target x_update =
                x_mode == TI_C54X_INDIRECT_DUAL_MOD0
                    ? TI_C54X_ADDRESS_UPDATE_NONE
                    : TI_C54X_ADDRESS_UPDATE_AR;
            const ti_c54x_address_update_target y_update =
                y_mode == TI_C54X_INDIRECT_DUAL_MOD0
                    ? TI_C54X_ADDRESS_UPDATE_NONE
                    : TI_C54X_ADDRESS_UPDATE_AR;

            check_operand(&result.x, UINT8_C(2), x_value, x_update,
                          x_mode == TI_C54X_INDIRECT_DUAL_MOD1
                              ? (uint16_t)(x_value - UINT16_C(1))
                              : (uint16_t)(x_value + UINT16_C(1)));
            check_operand(&result.y, UINT8_C(3), y_value, y_update,
                          y_mode == TI_C54X_INDIRECT_DUAL_MOD1
                              ? (uint16_t)(y_value - UINT16_C(1))
                              : (uint16_t)(y_value + UINT16_C(1)));
        }
    }
}

static void
test_distinct_selector_boundaries_are_transactional(void)
{
    const ti_c54x_indirect_dual_result x_underflow =
        ti_c54x_indirect_dual_word16(
            UINT8_C(2), UINT16_C(0), UINT8_C(3), UINT16_C(0x1234),
            TI_C54X_INDIRECT_DUAL_MOD1, TI_C54X_INDIRECT_DUAL_MOD0,
            UINT16_C(0), UINT16_C(0),
            TI_C54X_ADDRESS_ACCESS_READ);
    const ti_c54x_indirect_dual_result y_overflow =
        ti_c54x_indirect_dual_word16(
            UINT8_C(2), UINT16_C(0x1234), UINT8_C(3), UINT16_C(0xffff),
            TI_C54X_INDIRECT_DUAL_MOD0, TI_C54X_INDIRECT_DUAL_MOD2,
            UINT16_C(0), UINT16_C(0),
            TI_C54X_ADDRESS_ACCESS_READ);

    check_rejected(&x_underflow, TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
    check_rejected(&y_overflow, TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY);
}

static void
test_same_selector_uses_x_mode_for_one_update(void)
{
    const uint8_t ar_index = UINT8_C(4);
    const uint16_t value = UINT16_C(0x4000);

    {
        const ti_c54x_indirect_dual_result result =
            ti_c54x_indirect_dual_word16(
                ar_index, value, ar_index, value,
                TI_C54X_INDIRECT_DUAL_MOD1,
                TI_C54X_INDIRECT_DUAL_MOD1,
                UINT16_C(0), UINT16_C(0),
                TI_C54X_ADDRESS_ACCESS_READ);
        check_operand(&result.x, ar_index, value,
                      TI_C54X_ADDRESS_UPDATE_AR,
                      (uint16_t)(value - UINT16_C(1)));
        check_operand(&result.y, ar_index, value,
                      TI_C54X_ADDRESS_UPDATE_NONE, UINT16_C(0));
    }
    {
        const ti_c54x_indirect_dual_result result =
            ti_c54x_indirect_dual_word16(
                ar_index, value, ar_index, value,
                TI_C54X_INDIRECT_DUAL_MOD2,
                TI_C54X_INDIRECT_DUAL_MOD2,
                UINT16_C(0), UINT16_C(0),
                TI_C54X_ADDRESS_ACCESS_READ);
        check_operand(&result.x, ar_index, value,
                      TI_C54X_ADDRESS_UPDATE_AR,
                      (uint16_t)(value + UINT16_C(1)));
        check_operand(&result.y, ar_index, value,
                      TI_C54X_ADDRESS_UPDATE_NONE, UINT16_C(0));
    }
    /* The Y mode cannot add a second update or boundary when X owns the alias. */
    {
        const ti_c54x_indirect_dual_result result =
            ti_c54x_indirect_dual_word16(
                ar_index, UINT16_C(0), ar_index, UINT16_C(0),
                TI_C54X_INDIRECT_DUAL_MOD2,
                TI_C54X_INDIRECT_DUAL_MOD1,
                UINT16_C(0), UINT16_C(0),
                TI_C54X_ADDRESS_ACCESS_READ);
        check_operand(&result.x, ar_index, UINT16_C(0),
                      TI_C54X_ADDRESS_UPDATE_AR, UINT16_C(1));
        check_operand(&result.y, ar_index, UINT16_C(0),
                      TI_C54X_ADDRESS_UPDATE_NONE, UINT16_C(0));
    }
    {
        const ti_c54x_indirect_dual_result result =
            ti_c54x_indirect_dual_word16(
                ar_index, UINT16_C(0xffff), ar_index, UINT16_C(0xffff),
                TI_C54X_INDIRECT_DUAL_MOD0,
                TI_C54X_INDIRECT_DUAL_MOD2,
                UINT16_C(0), UINT16_C(0),
                TI_C54X_ADDRESS_ACCESS_READ);
        check_operand(&result.x, ar_index, UINT16_C(0xffff),
                      TI_C54X_ADDRESS_UPDATE_NONE, UINT16_C(0));
        check_operand(&result.y, ar_index, UINT16_C(0xffff),
                      TI_C54X_ADDRESS_UPDATE_NONE, UINT16_C(0));
    }
}

int
main(void)
{
    test_distinct_selectors_plan_independently();
    test_distinct_selector_boundaries_are_transactional();
    test_same_selector_uses_x_mode_for_one_update();
    puts("indirect_dual_modes: ok");
    return 0;
}
