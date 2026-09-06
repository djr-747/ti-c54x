/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY coverage for reverse-carry source
 * shape and policy validation, deterministic dependency ordering, and
 * bytewise transactional rejection.
 * Does not own: reverse-carry arithmetic, architectural storage, opcode
 * decode, buses, timing, diagnostics, or unresolved boundary policy.
 */

#include "sequencer/address_effect_apply.h"
#include "address/indirect_reverse_carry.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_address_result valid_result(void)
{
    return ti_c54x_indirect_reverse_carry_word16(
        UINT8_C(3), UINT16_C(0x0060), UINT16_C(0x0008),
        TI_C54X_INDIRECT_REVERSE_CARRY_MOD7,
        TI_C54X_ADDRESS_ACCESS_READ);
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
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x0060)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_C(0x0008)) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_arch_state mode_only(void)
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

static void test_intrinsic_shape_rejections_precede_state(void)
{
    ti_c54x_address_result result;
    ti_c54x_arch_state state;

    result = valid_result();
    state = (ti_c54x_arch_state){0};
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_NONE;
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.source.value.ar_reverse_carry.index = UINT8_C(0);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.source.value.ar_reverse_carry.index = UINT8_C(8);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.source.value.ar_reverse_carry.modification = UINT8_C(5);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.source.value.ar_reverse_carry.ar0_value_before =
        UINT16_C(0);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.operand.first_word = UINT16_C(0x0061);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.operand = ti_c54x_data_operand_long32(
        result.effect.operand.first_word);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.update.target = TI_C54X_ADDRESS_UPDATE_NONE;
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.update.phase = TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS;
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.update.ar_index = UINT8_C(2);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.update.value_before = UINT16_C(0x0061);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.update.value_after = UINT16_C(0x0060);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.state_policy = TI_C54X_ADDRESS_STATE_POLICY_NONE;
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);
}

static void test_phase_mode_and_source_precedence(void)
{
    ti_c54x_address_result result = valid_result();
    ti_c54x_arch_state state;

    state = (ti_c54x_arch_state){0};
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH);

    state = (ti_c54x_arch_state){0};
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE);

    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE);

    state = (ti_c54x_arch_state){0};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&state, UINT8_C(0)) ==
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

    state = mode_only();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x0061)) ==
                       TI_C54X_ARCH_STATE_OK);
    /* The selected AR is checked before the independent AR0 source. */
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_STALE_SOURCE);

    state = mode_only();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_C(0x0009)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);

    state = mode_only();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x0060)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);

    state = mode_only();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x0060)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(0), UINT16_C(0x0009)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_STALE_SOURCE);

    state = known_mode();
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                       TI_C54X_ADDRESS_APPLY_OK);
}

static void test_rejected_result_precedes_shape_and_state(void)
{
    ti_c54x_arch_state state = known_mode();
    ti_c54x_address_result result = valid_result();

    result.status = TI_C54X_ADDRESS_STATUS_UNRESOLVED_BOUNDARY;
    result.effect.source.kind = TI_C54X_ADDRESS_SOURCE_NONE;
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_REJECTED_RESULT);
}

int main(void)
{
    test_intrinsic_shape_rejections_precede_state();
    test_phase_mode_and_source_precedence();
    test_rejected_result_precedes_shape_and_state();
    puts("indirect_reverse_carry_effect_apply_contract: ok");
    return 0;
}
