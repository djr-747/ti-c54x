/*
 * Module contract
 *
 * Owns: PROJECT_CONTRACT_ONLY proof of validator argument/result/effect/phase
 * checks, zero-on-rejection output, exact commit descriptors, and read-only
 * state. Does not own: dependency-order vectors or architectural mutation.
 */

#include "sequencer/address_effect_validate.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state known_ar(uint8_t index, uint16_t value)
{
    ti_c54x_arch_state state = {0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&state, index, value) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_address_result ar_effect(uint8_t index, uint16_t before,
                                        ti_c54x_address_update_phase phase,
                                        uint16_t after)
{
    const bool has_update = phase != TI_C54X_ADDRESS_UPDATE_PHASE_NONE;
    const uint16_t effective =
        has_update && phase == TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS
            ? after
            : before;
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = {.width = TI_C54X_DATA_OPERAND_WIDTH_WORD16,
                        .first_word = effective,
                        .second_word = UINT16_C(0)},
            .source = {.kind = TI_C54X_ADDRESS_SOURCE_AR,
                       .value.ar = {.index = index, .value_before = before}},
            .state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0,
            .update = {
                .target = has_update ? TI_C54X_ADDRESS_UPDATE_AR
                                     : TI_C54X_ADDRESS_UPDATE_NONE,
                .phase = phase,
                .ar_index = has_update ? index : UINT8_C(0),
                .value_before = has_update ? before : UINT16_C(0),
                .value_after = has_update ? after : UINT16_C(0)}}};
    return result;
}

static void poison(ti_c54x_sequencer_address_commit *commit)
{
    (void)memset(commit, 0xA5, sizeof(*commit));
}

static void expect_zero(const ti_c54x_sequencer_address_commit *commit)
{
    const unsigned char *const bytes = (const unsigned char *)commit;
    size_t index;
    for (index = 0U; index < sizeof(*commit); ++index) {
        TI_C54X_TEST_CHECK(bytes[index] == 0U);
    }
}

static void expect_error(const ti_c54x_arch_state *before,
                         ti_c54x_arch_state *state,
                         const ti_c54x_address_result *result,
                         ti_c54x_address_update_phase phase,
                         ti_c54x_address_apply_status expected)
{
    ti_c54x_sequencer_address_commit commit;
    poison(&commit);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_validate_address_effect(
                           state, result, phase, &commit) == expected);
    expect_zero(&commit);
    TI_C54X_TEST_CHECK(memcmp(before, state, sizeof(*state)) == 0);
}

static void test_arguments_and_rejected_results(void)
{
    ti_c54x_arch_state state = known_ar(UINT8_C(2), UINT16_C(0x2200));
    const ti_c54x_arch_state before = state;
    const ti_c54x_address_result result = ar_effect(
        UINT8_C(2), UINT16_C(0x2200), TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        UINT16_C(0x2200));
    ti_c54x_sequencer_address_commit commit;

    poison(&commit);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_validate_address_effect(
                           NULL, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                           &commit) == TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT);
    expect_zero(&commit);
    poison(&commit);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_validate_address_effect(
                           &state, NULL, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                           &commit) == TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT);
    expect_zero(&commit);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_validate_address_effect(
                           &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                           NULL) == TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&before, &state, sizeof(state)) == 0);
    expect_error(&before, &state, &result,
                 (ti_c54x_address_update_phase)-1,
                 TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT);
    expect_error(&before, &state, &result,
                 (ti_c54x_address_update_phase)3,
                 TI_C54X_ADDRESS_APPLY_INVALID_ARGUMENT);

    {
        ti_c54x_address_result rejected = result;
        rejected.status = TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS;
        rejected.effect.source.kind = (ti_c54x_address_source_kind)99;
        expect_error(&before, &state, &rejected,
                     TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_REJECTED_RESULT);
    }
}

static void test_intrinsic_and_phase_errors(void)
{
    ti_c54x_arch_state state = known_ar(UINT8_C(2), UINT16_C(0x2200));
    const ti_c54x_arch_state before = state;
    ti_c54x_address_result result = ar_effect(
        UINT8_C(2), UINT16_C(0x2200), TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        UINT16_C(0x2200));

    result.effect.source.kind = (ti_c54x_address_source_kind)99;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                 TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = ar_effect(UINT8_C(2), UINT16_C(0x2200),
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                       UINT16_C(0x2200));
    result.effect.operand.width = (ti_c54x_data_operand_width)99;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                 TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = ar_effect(UINT8_C(2), UINT16_C(0x2200),
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                       UINT16_C(0x2200));
    result.effect.update.target = (ti_c54x_address_update_target)99;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                 TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = ar_effect(UINT8_C(2), UINT16_C(0x2200),
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                       UINT16_C(0x2200));
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0;
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                 TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = ar_effect(UINT8_C(2), UINT16_C(0x2200),
                       TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                       UINT16_C(0x2201));
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                 TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH);
    result.effect.operand.first_word = UINT16_C(0x2200);
    expect_error(&before, &state, &result,
                 TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                 TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
}

static void test_exact_descriptors_and_nonmutation(void)
{
    ti_c54x_sequencer_address_commit commit;
    ti_c54x_arch_state state = known_ar(UINT8_C(0), UINT16_C(0x2200));
    ti_c54x_arch_state before = state;
    ti_c54x_address_result result = ar_effect(
        UINT8_C(0), UINT16_C(0x2200), TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
        UINT16_C(0x2200));

    poison(&commit);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_validate_address_effect(
                           &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                           &commit) == TI_C54X_ADDRESS_APPLY_OK);
    expect_zero(&commit);
    TI_C54X_TEST_CHECK(memcmp(&before, &state, sizeof(state)) == 0);

    state = known_ar(UINT8_C(3), UINT16_C(0x1200));
    before = state;
    result = ar_effect(UINT8_C(3), UINT16_C(0x1200),
                       TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                       UINT16_C(0x1201));
    poison(&commit);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_validate_address_effect(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                           &commit) == TI_C54X_ADDRESS_APPLY_OK);
    TI_C54X_TEST_CHECK(commit.target == TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK_U64(commit.ar_index, UINT8_C(3));
    TI_C54X_TEST_CHECK_U64(commit.value_after, UINT16_C(0x1201));
    TI_C54X_TEST_CHECK(memcmp(&before, &state, sizeof(state)) == 0);

    state = known_ar(UINT8_C(0), UINT16_C(0x2200));
    before = state;
    result = ar_effect(UINT8_C(0), UINT16_C(0x2200),
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                       UINT16_C(0x21FF));
    poison(&commit);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_validate_address_effect(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                           &commit) == TI_C54X_ADDRESS_APPLY_OK);
    TI_C54X_TEST_CHECK(commit.target == TI_C54X_ADDRESS_UPDATE_AR);
    TI_C54X_TEST_CHECK_U64(commit.ar_index, UINT8_C(0));
    TI_C54X_TEST_CHECK_U64(commit.value_after, UINT16_C(0x21FF));
    TI_C54X_TEST_CHECK(memcmp(&before, &state, sizeof(state)) == 0);
}

int main(void)
{
    test_arguments_and_rejected_results();
    test_intrinsic_and_phase_errors();
    test_exact_descriptors_and_nonmutation();
    puts("address_effect_validate_contract: ok");
    return 0;
}
