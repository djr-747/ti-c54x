/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT_ONLY proof of validation dependency ordering for
 * CMPT/ARP, selected-AR then AR0, and CPL then DP/SP. Every rejection also
 * proves zeroed output and bytewise state nonmutation.
 * Does not own: address planning, intrinsic shape rules, architectural
 * mutation, opcode decoding, timing, buses, diagnostics, or public ABI.
 */

#include "sequencer/address_effect_validate.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state mode_state(void)
{
    ti_c54x_arch_state state = {0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_address_result indexed_effect(void)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = {.width = TI_C54X_DATA_OPERAND_WIDTH_WORD16,
                        .first_word = UINT16_C(0x2200)},
            .source = {
                .kind = TI_C54X_ADDRESS_SOURCE_AR_INDEXED,
                .value.ar_indexed = {.index = UINT8_C(2),
                                      .value_before = UINT16_C(0x2200),
                                      .ar0_value_before = UINT16_C(0x0040)}},
            .state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0,
            .update = {.target = TI_C54X_ADDRESS_UPDATE_AR,
                       .phase = TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                       .ar_index = UINT8_C(2),
                       .value_before = UINT16_C(0x2200),
                       .value_after = UINT16_C(0x2201)}}};
    return result;
}

static ti_c54x_address_result direct_dp_effect(void)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = {.width = TI_C54X_DATA_OPERAND_WIDTH_WORD16,
                .first_word = UINT16_C(0x9191)},
            .source = {.kind = TI_C54X_ADDRESS_SOURCE_DIRECT_DP,
                       .value.direct_dp = {.dp9 = UINT16_C(0x0123),
                                            .dma7 = UINT8_C(0x11)}},
            .state_policy = TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0,
            .update = {.target = TI_C54X_ADDRESS_UPDATE_NONE,
                       .phase = TI_C54X_ADDRESS_UPDATE_PHASE_NONE}}};
    return result;
}

static ti_c54x_address_result direct_sp_effect(void)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = {.width = TI_C54X_DATA_OPERAND_WIDTH_WORD16,
                        .first_word = UINT16_C(0x1203)},
            .source = {.kind = TI_C54X_ADDRESS_SOURCE_DIRECT_SP,
                       .value.direct_sp = {.sp16 = UINT16_C(0x1200),
                                            .dma7 = UINT8_C(3)}},
            .state_policy = TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL1,
            .update = {.target = TI_C54X_ADDRESS_UPDATE_NONE,
                       .phase = TI_C54X_ADDRESS_UPDATE_PHASE_NONE}}};
    return result;
}

static void expect_error(const ti_c54x_arch_state *before,
                         ti_c54x_arch_state *state,
                         const ti_c54x_address_result *result,
                         ti_c54x_address_apply_status expected)
{
    ti_c54x_sequencer_address_commit commit;
    const unsigned char *bytes;
    size_t index;

    (void)memset(&commit, 0xA5, sizeof(commit));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_validate_address_effect(
                           state, result, result->effect.update.phase,
                           &commit) == expected);
    bytes = (const unsigned char *)&commit;
    for (index = 0U; index < sizeof(commit); ++index) {
        TI_C54X_TEST_CHECK(bytes[index] == 0U);
    }
    TI_C54X_TEST_CHECK(memcmp(before, state, sizeof(*state)) == 0);
}

static void test_cmpt_arp_order(void)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = {.width = TI_C54X_DATA_OPERAND_WIDTH_WORD16,
                        .first_word = UINT16_C(0x2200)},
            .source = {.kind = TI_C54X_ADDRESS_SOURCE_AR,
                       .value.ar = {.index = UINT8_C(2),
                                    .value_before = UINT16_C(0x2200)}},
            .state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0,
            .update = {.target = TI_C54X_ADDRESS_UPDATE_NONE,
                       .phase = TI_C54X_ADDRESS_UPDATE_PHASE_NONE}}};
    ti_c54x_arch_state state = {0};
    ti_c54x_arch_state before;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(3)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE);

    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE);
}

static void test_selected_ar_then_ar0_order(void)
{
    const ti_c54x_address_result result = indexed_effect();
    ti_c54x_arch_state state = mode_state();
    ti_c54x_arch_state before;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_C(0x0040)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);

    state = mode_state();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2201)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_STALE_SOURCE);

    state = mode_state();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);

    state = mode_state();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_C(0x0041)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_STALE_SOURCE);
}

static void test_cpl_then_dp_sp_order(void)
{
    ti_c54x_address_result result = direct_dp_effect();
    ti_c54x_arch_state state = {0};
    ti_c54x_arch_state before;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(
                           &state, UINT16_C(0x0123)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE);
    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION);
    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(
                           &state, UINT16_C(0x0122)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_STALE_SOURCE);

    result = direct_sp_effect();
    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(
                           &state, UINT16_C(0x1200)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE);
    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION);
    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(
                           &state, UINT16_C(0x1201)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_APPLY_STALE_SOURCE);
}

int main(void)
{
    test_cmpt_arp_order();
    test_selected_ar_then_ar0_order();
    test_cpl_then_dp_sp_order();
    puts("address_effect_dependency_contract: ok");
    return 0;
}
