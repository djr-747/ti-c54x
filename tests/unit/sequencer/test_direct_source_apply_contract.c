/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for tagged direct DP/SP
 * validation, exact policy and mode-state precedence, unknown/stale source
 * handling, and bytewise transactional rejection.
 * Does not own: direct-address planning, architectural register layout,
 * timing, bus transfers, opcode decoding, or diagnostics.
 */

#include "sequencer/address_effect_apply.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_address_result
valid_dp(uint16_t dp9, uint8_t dma7)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = ti_c54x_data_operand_word16(
                (uint16_t)((dp9 << 7U) | dma7)),
            .source = {
                .kind = TI_C54X_ADDRESS_SOURCE_DIRECT_DP,
                .value.direct_dp = {.dp9 = dp9, .dma7 = dma7}
            },
            .state_policy = TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0,
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
valid_sp(uint16_t sp16, uint8_t dma7)
{
    const ti_c54x_address_result result = {
        .status = TI_C54X_ADDRESS_STATUS_OK,
        .effect = {
            .operand = ti_c54x_data_operand_word16(
                (uint16_t)((uint32_t)sp16 + (uint32_t)dma7)),
            .source = {
                .kind = TI_C54X_ADDRESS_SOURCE_DIRECT_SP,
                .value.direct_sp = {.sp16 = sp16, .dma7 = dma7}
            },
            .state_policy = TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL1,
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
expect_unchanged(ti_c54x_arch_state *state,
                 const ti_c54x_address_result *result,
                 ti_c54x_address_update_phase phase,
                 ti_c54x_address_apply_status expected)
{
    const ti_c54x_arch_state before = *state;

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           state, result, phase) == expected);
    TI_C54X_TEST_CHECK(memcmp(&before, state, sizeof(before)) == 0);
}

static void
test_malformed_direct_payloads_precede_mode_and_source_reads(void)
{
    ti_c54x_arch_state state = {0};
    ti_c54x_address_result result = valid_dp(UINT16_C(3), UINT8_C(5));

    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_NONE;
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_dp(UINT16_C(3), UINT8_C(5));
    result.effect.source.kind = (ti_c54x_address_source_kind)-1;
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_dp(UINT16_C(3), UINT8_C(5));
    result.effect.source.kind = (ti_c54x_address_source_kind)5;
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_dp(UINT16_C(3), UINT8_C(5));
    result.effect.source.value.direct_dp.dp9 = UINT16_C(0x200);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_dp(UINT16_C(3), UINT8_C(5));
    result.effect.source.value.direct_dp.dma7 = UINT8_C(0x80);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_dp(UINT16_C(3), UINT8_C(5));
    result.effect.operand.first_word = UINT16_C(0x9999);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_dp(UINT16_C(3), UINT8_C(4));
    result.effect.operand = ti_c54x_data_operand_long32(
        result.effect.operand.first_word);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_dp(UINT16_C(3), UINT8_C(5));
    result.effect.update.value_before = UINT16_C(1);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_dp(UINT16_C(3), UINT8_C(5));
    result.effect.update.target = TI_C54X_ADDRESS_UPDATE_AR;
    result.effect.update.phase = TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS;
    result.effect.update.ar_index = UINT8_C(0);
    result.effect.update.value_before = UINT16_C(0);
    result.effect.update.value_after = UINT16_C(1);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_dp(UINT16_C(3), UINT8_C(5));
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL1;
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_dp(UINT16_C(3), UINT8_C(5));
    result.effect.state_policy = (ti_c54x_address_state_policy)4;
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_dp(UINT16_C(3), UINT8_C(5));
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH);

    result = valid_dp(UINT16_C(3), UINT8_C(5));
    result.status = TI_C54X_ADDRESS_STATUS_UNSUPPORTED_ACCESS;
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_NONE;
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_REJECTED_RESULT);
}

static void
test_malformed_sp_payloads_precede_mode_and_source_reads(void)
{
    ti_c54x_arch_state state = {0};
    ti_c54x_address_result result = valid_sp(UINT16_C(0x1200), UINT8_C(7));

    result.effect.source.value.direct_sp.dma7 = UINT8_C(0x80);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_sp(UINT16_MAX, UINT8_C(1));
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_sp(UINT16_C(0x1200), UINT8_C(7));
    result.effect.operand.first_word = UINT16_C(0x1208);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_sp(UINT16_C(0x1200), UINT8_C(7));
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0;
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
}

static void
test_direct_dp_mode_then_source_precedence(void)
{
    ti_c54x_address_result result = valid_dp(UINT16_C(3), UINT8_C(5));
    ti_c54x_arch_state state = {0};

    /* CPL is an exact mode precondition and is checked before DP knownness. */
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(
                           &state, UINT8_C(0)) == TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(
                           &state, UINT16_C(4)) == TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_STALE_SOURCE);

    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(
                           &state, UINT8_C(1)) == TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION);

    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(
                           &state, UINT8_C(0)) == TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(
                           &state, UINT16_C(3)) == TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_OK);
}

static void
test_direct_sp_mode_then_source_precedence(void)
{
    ti_c54x_address_result result = valid_sp(UINT16_C(0x1200), UINT8_C(7));
    ti_c54x_arch_state state = {0};

    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(
                           &state, UINT8_C(1)) == TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(
                           &state, UINT16_C(0x1201)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_STALE_SOURCE);

    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(
                           &state, UINT8_C(0)) == TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_MODE_STATE_VIOLATION);

    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cpl(
                           &state, UINT8_C(1)) == TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(
                           &state, UINT16_C(0x1200)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_OK);
}

int
main(void)
{
    test_malformed_direct_payloads_precede_mode_and_source_reads();
    test_malformed_sp_payloads_precede_mode_and_source_reads();
    test_direct_dp_mode_then_source_precedence();
    test_direct_sp_mode_then_source_precedence();
    puts("direct_source_apply_contract: ok");
    return 0;
}
