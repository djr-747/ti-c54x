/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for validation ordering,
 * phase/effective-address relationships, knownness, stale-state rejection,
 * and whole-state transactional nonmutation of address-effect application.
 * Does not own: TI instruction semantics, address planning, CPU lifecycle,
 * bus transfers, timing, diagnostics, or public ABI compatibility.
 */

#include "sequencer/address_effect_apply.h"
#include "address/indirect_single.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state known_state(uint8_t index, uint16_t value)
{
    ti_c54x_arch_state state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, index, value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_address_result valid_none(uint8_t index, uint16_t address)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .effective_address_word = address,
            .selected_ar_index = index,
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

static ti_c54x_address_result valid_update(uint8_t index,
                                           ti_c54x_address_update_phase phase,
                                           uint16_t before,
                                           uint16_t after,
                                           uint16_t effective_address)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .effective_address_word = effective_address,
            .selected_ar_index = index,
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

static void expect_rejection_unchanged(
    const ti_c54x_arch_state *before,
    ti_c54x_arch_state *state,
    const ti_c54x_address_result *result,
    ti_c54x_address_update_phase requested,
    ti_c54x_address_apply_status expected)
{
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_apply_address_result(
            state, result, requested) == expected);
    TI_C54X_TEST_CHECK(memcmp(before, state, sizeof(*state)) == 0);
}

static void test_null_and_requested_phase_arguments(void)
{
    ti_c54x_arch_state state = known_state(UINT8_C(2), UINT16_C(0x2200));
    const ti_c54x_arch_state before = state;
    const ti_c54x_address_result result = valid_none(
        UINT8_C(2), UINT16_C(0x2200));

    expect_rejection_unchanged(
        &before, &state, NULL, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT);
    expect_rejection_unchanged(
        &before, &state, &result,
        (ti_c54x_address_update_phase)-1,
        TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT);
    expect_rejection_unchanged(
        &before, &state, &result,
        (ti_c54x_address_update_phase)3,
        TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_apply_address_result(
            NULL, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
        TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT);
}

static void test_non_ok_result_is_rejected_before_payload(void)
{
    ti_c54x_arch_state state = known_state(UINT8_C(2), UINT16_C(0x2200));
    const ti_c54x_arch_state before = state;
    ti_c54x_address_result result = valid_update(
        UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        UINT16_C(0x2200), UINT16_C(0x2201), UINT16_C(0x2201));

    result.status = TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS;
    result.effect.selected_ar_index = UINT8_C(255);
    result.effect.update.target = (ti_c54x_address_update_target)-1;
    result.effect.update.phase = (ti_c54x_address_update_phase)99;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_REJECTED_RESULT);
}

static void test_every_non_ok_status_is_rejected_before_payload(void)
{
    const ti_c54x_address_status statuses[] = {
        TI_C54X_ADDRESS_STATUS_INVALID_ARGUMENT,
        TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS,
        TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY,
        (ti_c54x_address_status)-1,
        (ti_c54x_address_status)99
    };

    for (size_t status_index = 0U;
         status_index < sizeof(statuses) / sizeof(statuses[0]);
         ++status_index) {
        ti_c54x_arch_state state = known_state(UINT8_C(2), UINT16_C(0x2200));
        const ti_c54x_arch_state before = state;
        ti_c54x_address_result result = valid_none(
            UINT8_C(2), UINT16_C(0x2200));

        result.status = statuses[status_index];
        result.effect.selected_ar_index = UINT8_C(255);
        result.effect.update.target =
            (ti_c54x_address_update_target)99;
        result.effect.update.phase =
            (ti_c54x_address_update_phase)-1;
        result.effect.update.ar_index = UINT8_C(255);
        result.effect.update.value_before = UINT16_C(0xFFFF);
        result.effect.update.value_after = UINT16_C(0xFFFF);
        expect_rejection_unchanged(
            &before, &state, &result,
            TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
            TI_C54X_ADDRESS_APPLY_REJECTED_RESULT);
    }
}

static void test_validation_precedence(void)
{
    ti_c54x_arch_state state;
    ti_c54x_arch_state before;
    ti_c54x_address_result result;

    state = known_state(UINT8_C(2), UINT16_C(0x2200));
    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    result.status = TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS;
    result.effect.selected_ar_index = UINT8_C(255);
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        (ti_c54x_address_update_phase)99,
        TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT);

    state = known_state(UINT8_C(2), UINT16_C(0x2200));
    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    result.effect.update.phase = (ti_c54x_address_update_phase)99;
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2200));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
        TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2200));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
}

static void test_invalid_intrinsic_effect_shape(void)
{
    ti_c54x_arch_state state = known_state(UINT8_C(2), UINT16_C(0x2200));
    ti_c54x_arch_state before;
    ti_c54x_address_result result;

    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    result.effect.selected_ar_index = UINT8_C(8);
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result.effect.selected_ar_index = UINT8_C(255);
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    result.effect.update.target = (ti_c54x_address_update_target)2;
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result.effect.update.target = (ti_c54x_address_update_target)-1;
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    result.effect.update.phase = (ti_c54x_address_update_phase)3;
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result.effect.update.phase = (ti_c54x_address_update_phase)-1;
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    result.effect.update.ar_index = UINT8_C(1);
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    result.effect.update.value_before = UINT16_C(1);
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    result.effect.update.value_after = UINT16_C(1);
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_update(UINT8_C(2),
                          TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2201));
    result.effect.update.ar_index = UINT8_C(8);
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result.effect.update.ar_index = UINT8_C(255);
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_update(UINT8_C(2),
                          TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2201));
    result.effect.update.phase = (ti_c54x_address_update_phase)3;
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result.effect.update.phase = (ti_c54x_address_update_phase)-1;
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2200));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
}

static void test_index_phase_and_address_relationships(void)
{
    ti_c54x_arch_state state = known_state(UINT8_C(2), UINT16_C(0x2200));
    ti_c54x_arch_state before;
    ti_c54x_address_result result;

    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2201));
    result.effect.update.ar_index = UINT8_C(3);
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2200));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2201));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
        TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH);
    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2201));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
        TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH);
    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2200));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH);
}

static void test_unknown_and_stale_are_transactional(void)
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
        TI_C54X_ADDRESS_APPLY_UNKNOWN_AR);

    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2201));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_UNKNOWN_AR);

    state = known_state(UINT8_C(2), UINT16_C(0x2201));
    result = valid_none(UINT8_C(2), UINT16_C(0x2200));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        TI_C54X_ADDRESS_APPLY_STALE_AR);

    state = known_state(UINT8_C(2), UINT16_C(0x2201));
    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2201));
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
        TI_C54X_ADDRESS_APPLY_STALE_AR);

    state = known_state(UINT8_C(2), UINT16_C(0x2200));
    result = valid_update(UINT8_C(2), TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                          UINT16_C(0x2200), UINT16_C(0x2201),
                          UINT16_C(0x2200));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_apply_address_result(
            &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
        TI_C54X_ADDRESS_APPLY_OK);
    before = state;
    expect_rejection_unchanged(
        &before, &state, &result,
        TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
        TI_C54X_ADDRESS_APPLY_STALE_AR);
}

static void test_equal_before_after_is_valid_when_relation_holds(void)
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

    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_apply_address_result(
            &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS) ==
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
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_apply_address_result(
            &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
        TI_C54X_ADDRESS_APPLY_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                           &state, UINT8_C(2), &observed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, UINT16_C(0x2200));
    TI_C54X_TEST_CHECK(memcmp(&state, &expected_state, sizeof(state)) == 0);
}

int main(void)
{
    test_null_and_requested_phase_arguments();
    test_non_ok_result_is_rejected_before_payload();
    test_every_non_ok_status_is_rejected_before_payload();
    test_validation_precedence();
    test_invalid_intrinsic_effect_shape();
    test_index_phase_and_address_relationships();
    test_unknown_and_stale_are_transactional();
    test_equal_before_after_is_valid_when_relation_holds();
    puts("address_effect_apply_contract: ok");
    return 0;
}
