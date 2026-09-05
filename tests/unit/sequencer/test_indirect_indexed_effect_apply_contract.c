/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for indexed-source arm and
 * update validation, CMPT0/ARP0 precedence, selected-target then AR0 source
 * checks, and bytewise transactional rejection.
 * Does not own: planner arithmetic, architectural storage, decode, buses,
 * timing, diagnostics, or unresolved boundary policy.
 */

#include "sequencer/address_effect_apply.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_address_result valid_indexed(uint8_t index,
                                            uint16_t target_before,
                                            uint16_t ar0_before,
                                            uint16_t target_after)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = ti_c54x_data_operand_word16(target_before),
            .source = {
                .kind = TI_C54X_ADDRESS_SOURCE_AR_INDEXED,
                .value.ar_indexed = {
                    .index = index,
                    .value_before = target_before,
                    .ar0_value_before = ar0_before
                }
            },
            .state_policy = TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0,
            .update = {
                .target = TI_C54X_ADDRESS_UPDATE_AR,
                .phase = TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                .ar_index = index,
                .value_before = target_before,
                .value_after = target_after
            }
        }
    };
    return result;
}

static ti_c54x_arch_state known_mode(void)
{
    ti_c54x_arch_state state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static void expect_unchanged(ti_c54x_arch_state *state,
                              const ti_c54x_address_result *result,
                              ti_c54x_address_update_phase phase,
                              ti_c54x_address_apply_status expected)
{
    const ti_c54x_arch_state before = *state;

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           state, result, phase) == expected);
    TI_C54X_TEST_CHECK(memcmp(&before, state, sizeof(before)) == 0);
}

static void test_malformed_indexed_arm_and_policy(void)
{
    ti_c54x_arch_state state = known_mode();
    ti_c54x_address_result result =
        valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                       UINT16_C(0x2204));

    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_AR;
    result.effect.source.value.ar.index = UINT8_C(8);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                           UINT16_C(0x2204));
    result.effect.source.value.ar_indexed.index = UINT8_C(0);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                           UINT16_C(0x2204));
    result.effect.source.value.ar_indexed.index = UINT8_C(8);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                           UINT16_C(0x2204));
    result.effect.operand.first_word = UINT16_C(0x2201);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                           UINT16_C(0x2204));
    result.effect.operand = ti_c54x_data_operand_long32(UINT16_C(0x2200));
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                           UINT16_C(0x2204));
    result.effect.update.target = TI_C54X_ADDRESS_UPDATE_NONE;
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                           UINT16_C(0x2204));
    result.effect.update.phase = TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS;
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                           UINT16_C(0x2204));
    result.effect.update.ar_index = UINT8_C(4);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                           UINT16_C(0x2204));
    result.effect.update.value_before = UINT16_C(0x2201);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                           UINT16_C(0x2204));
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_NONE;
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
    result = valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                           UINT16_C(0x2204));
    result.status = TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY;
    result.effect.source.value.ar_indexed.index = UINT8_C(0);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_REJECTED_RESULT);
    result = valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                           UINT16_C(0x2204));
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH);
}

static void test_mode_before_target_and_ar0_sources(void)
{
    ti_c54x_address_result result =
        valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                      UINT16_C(0x2204));
    ti_c54x_arch_state state;

    state = (ti_c54x_arch_state){0};
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE);

    state = known_mode();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION);
    state = known_mode();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION);

    state = known_mode();
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);
}

static void test_target_precedence_stale_and_success(void)
{
    ti_c54x_address_result result =
        valid_indexed(UINT8_C(3), UINT16_C(0x2200), UINT16_C(4),
                      UINT16_C(0x2204));
    ti_c54x_arch_state state = known_mode();
    uint16_t observed;

    /* A stale selected target is reported before an unknown AR0 source. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x2201)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_STALE_SOURCE);

    state = known_mode();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_C(5)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_STALE_SOURCE);

    state = known_mode();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x2200)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_C(4)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                       TI_C54X_ADDRESS_APPLY_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                           &state, UINT8_C(3), &observed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, UINT16_C(0x2204));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                           &state, UINT8_C(0), &observed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, UINT16_C(4));
}

int main(void)
{
    test_malformed_indexed_arm_and_policy();
    test_mode_before_target_and_ar0_sources();
    test_target_precedence_stale_and_success();
    puts("indirect_indexed_effect_apply_contract: ok");
    return 0;
}
