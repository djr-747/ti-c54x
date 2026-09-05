/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for address-effect state
 * policy validation, status precedence, and bytewise transactional rejection.
 * Does not own: TI mode evidence, register layout, address planning, opcode
 * decoding, bus mapping, timing, diagnostics, or state storage internals.
 */

#include "sequencer/address_effect_apply.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_address_result
effect_with_policy(ti_c54x_address_state_policy policy,
                   uint8_t index,
                   uint16_t address)
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
            .state_policy = policy,
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

static void
expect_unchanged(const ti_c54x_arch_state *before,
                 ti_c54x_arch_state *state,
                 const ti_c54x_address_result *result,
                 ti_c54x_address_apply_status expected,
                 ti_c54x_address_update_phase phase)
{
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_apply_address_result(state, result, phase) ==
        expected);
    TI_C54X_TEST_CHECK(memcmp(before, state, sizeof(*state)) == 0);
}

static void
test_unknown_policy_values_are_intrinsic_errors(void)
{
    const ti_c54x_address_state_policy policies[] = {
        (ti_c54x_address_state_policy)-1,
        (ti_c54x_address_state_policy)2,
        (ti_c54x_address_state_policy)UINT8_C(255)
    };
    size_t policy_index;

    for (policy_index = 0U;
         policy_index < sizeof(policies) / sizeof(policies[0]);
         ++policy_index) {
        ti_c54x_arch_state state = {0};
        ti_c54x_arch_state before;
        ti_c54x_address_result result = effect_with_policy(
            policies[policy_index], UINT8_C(0), UINT16_C(0x2200));

        before = state;
        expect_unchanged(&before, &state, &result,
                         TI_C54X_ADDRESS_APPLY_INVALID_EFFECT,
                         TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
    }
}

static void
test_unknown_cmpt_and_arp_are_distinct_mode_state_failures(void)
{
    ti_c54x_address_result result = effect_with_policy(
        TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0,
        UINT8_C(2), UINT16_C(0x2200));
    ti_c54x_arch_state state = {0};
    ti_c54x_arch_state before;

    /* Keep the selected AR known so only unknown CMPT/ARP can be reported. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_unchanged(&before, &state, &result,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE,
                     TI_C54X_ADDRESS_UPDATE_PHASE_NONE);

    /* Unknown CMPT outranks a known, violating nonzero ARP. */
    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(3)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_unchanged(&before, &state, &result,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE,
                     TI_C54X_ADDRESS_UPDATE_PHASE_NONE);

    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_unchanged(&before, &state, &result,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE,
                     TI_C54X_ADDRESS_UPDATE_PHASE_NONE);

    /* Unknown ARP outranks a known, violating nonzero CMPT. */
    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_unchanged(&before, &state, &result,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE,
                     TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
}

static void
test_mode_value_violation_precedes_ar_unknown_and_never_repairs(void)
{
    const uint8_t cmpt_values[] = {UINT8_C(1), UINT8_C(0)};
    const uint8_t arp_values[] = {UINT8_C(0), UINT8_C(1)};
    size_t value_index;

    for (value_index = 0U; value_index < 2U; ++value_index) {
        ti_c54x_arch_state state = {0};
        ti_c54x_arch_state before;
        uint8_t observed_cmpt = UINT8_C(0);
        uint8_t observed_arp = UINT8_C(0);
        const ti_c54x_address_result result = effect_with_policy(
            TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0,
            UINT8_C(2), UINT16_C(0x2200));

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(
                               &state, cmpt_values[value_index]) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(
                               &state, arp_values[value_index]) ==
                           TI_C54X_ARCH_STATE_OK);
        before = state;
        expect_unchanged(&before, &state, &result,
                         TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION,
                         TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_cmpt(
                               &state, &observed_cmpt) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(observed_cmpt, cmpt_values[value_index]);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_arp(
                               &state, &observed_arp) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK_U64(observed_arp, arp_values[value_index]);
    }
}

static void
test_none_policy_does_not_consult_mode_state(void)
{
    ti_c54x_arch_state state = {0};
    ti_c54x_arch_state before;
    const ti_c54x_address_result result = effect_with_policy(
        TI_C54X_ADDRESS_STATE_POLICY_NONE,
        UINT8_C(2), UINT16_C(0x2200));

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(7)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_apply_address_result(
            &state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
        TI_C54X_ADDRESS_APPLY_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &before, sizeof(state)) == 0);
}

static void
test_policy_failures_precede_stale_ar(void)
{
    ti_c54x_arch_state state = {0};
    ti_c54x_arch_state before;
    ti_c54x_address_result result = effect_with_policy(
        (ti_c54x_address_state_policy)2, UINT8_C(2), UINT16_C(0x2200));

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(2), UINT16_C(0x1100)) ==
                       TI_C54X_ARCH_STATE_OK);
    before = state;
    expect_unchanged(&before, &state, &result,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT,
                     TI_C54X_ADDRESS_UPDATE_PHASE_NONE);
}

int
main(void)
{
    test_unknown_policy_values_are_intrinsic_errors();
    test_unknown_cmpt_and_arp_are_distinct_mode_state_failures();
    test_mode_value_violation_precedes_ar_unknown_and_never_repairs();
    test_none_policy_does_not_consult_mode_state();
    test_policy_failures_precede_stale_ar();
    puts("address_state_policy_contract: ok");
    return 0;
}
