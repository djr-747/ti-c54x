/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for unknown/stale
 * auxiliary-register state and equal-value AR updates at the transactional
 * address-effect boundary.
 * Does not own: address planning, intrinsic effect-shape validation, phase
 * precedence, CPU lifecycle, bus transfers, timing, opcode decoding, or
 * diagnostics. State is exercised only through its checked accessors.
 */

#include "sequencer/address_effect_apply.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state
known_state(uint8_t index, uint16_t value)
{
    ti_c54x_arch_state state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, index, value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_address_result
valid_none(uint8_t index, uint16_t address)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = {
                .width = TI_C54X_DATA_OPERAND_WIDTH_WORD16,
                .first_word = address,
                .second_word = UINT16_C(0)
            },
            .source = {
                .kind = TI_C54X_ADDRESS_SOURCE_AR,
                .value.ar = {.index = index, .value_before = address}
            },
            .state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0,
            .update = {
                .target = TI_C54X_ADDRESS_UPDATE_NONE,
                .phase = TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                .ar_index = UINT8_C(0),
                .value_before = UINT16_C(0),
                .value_after = UINT16_C(0)
            }
        }
    };
    return result;
}

static ti_c54x_address_result
valid_update(uint8_t index,
             ti_c54x_address_update_phase phase,
             uint16_t before,
             uint16_t after,
             uint16_t effective_address)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = {
                .width = TI_C54X_DATA_OPERAND_WIDTH_WORD16,
                .first_word = effective_address,
                .second_word = UINT16_C(0)
            },
            .source = {
                .kind = TI_C54X_ADDRESS_SOURCE_AR,
                .value.ar = {.index = index, .value_before = before}
            },
            .state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0,
            .update = {
                .target = TI_C54X_ADDRESS_UPDATE_AR,
                .phase = phase,
                .ar_index = index,
                .value_before = before,
                .value_after = after
            }
        }
    };
    return result;
}

static void
expect_rejection_unchanged(const ti_c54x_arch_state *before,
                           ti_c54x_arch_state *state,
                           const ti_c54x_address_result *result,
                           ti_c54x_address_update_phase requested,
                           ti_c54x_address_apply_status expected)
{
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           state, result, requested) == expected);
    TI_C54X_TEST_CHECK(memcmp(before, state, sizeof(*state)) == 0);
}

static void
test_unknown_and_stale_are_transactional(void)
{
    ti_c54x_arch_state state;
    ti_c54x_arch_state before;
    ti_c54x_address_result result;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);

    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2201));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);

    state = known_state(UINT8_C(2), UINT16_C(0x2201));
    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_STALE_SOURCE);

    state = known_state(UINT8_C(2), UINT16_C(0x2201));
    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2201));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_STALE_SOURCE);

    state = known_state(UINT8_C(2), UINT16_C(0x2200));
    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2200));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                       TI_C54X_ADDRESS_APPLY_OK);
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
        TI_C54X_ADDRESS_APPLY_STALE_SOURCE);
}

static void
test_equal_before_after_is_valid_when_relation_holds(void)
{
    ti_c54x_arch_state state = known_state(UINT8_C(2), UINT16_C(0x2200));
    ti_c54x_arch_state expected_state;
    uint16_t observed = UINT16_C(0);
    ti_c54x_address_result result = valid_update(
        UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        UINT16_C(0x2200), UINT16_C(0x2200), UINT16_C(0x2200));
    expected_state = state;
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &expected_state, UINT8_C(2), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS) ==
                       TI_C54X_ADDRESS_APPLY_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                           &state, UINT8_C(2), &observed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, UINT16_C(0x2200));
    TI_C54X_TEST_CHECK(memcmp(&state, &expected_state, sizeof(state)) == 0);

    state = known_state(UINT8_C(2), UINT16_C(0x2200));
    expected_state = state;
    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2200),
                          UINT16_C(0x2200));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &expected_state, UINT8_C(2), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                       TI_C54X_ADDRESS_APPLY_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                           &state, UINT8_C(2), &observed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, UINT16_C(0x2200));
    TI_C54X_TEST_CHECK(memcmp(&state, &expected_state, sizeof(state)) == 0);
}

int
main(void)
{
    test_unknown_and_stale_are_transactional();
    test_equal_before_after_is_valid_when_relation_holds();
    puts("address_effect_state_contract: ok");
    return 0;
}
