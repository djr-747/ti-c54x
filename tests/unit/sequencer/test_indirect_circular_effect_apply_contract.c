/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proof for circular-effect intrinsic
 * shape validation, dependency order (CMPT, ARP, selected AR, then BK),
 * unknown/stale precedence, the exact selected-AR mutation boundary, and
 * bytewise transactional rejection. BK is never an application target.
 * Does not own: circular arithmetic, architectural evidence, opcode decode,
 * buses, timing, diagnostics, or valid-domain planner coverage.
 */

#include "sequencer/address_effect_apply.h"
#include "address/indirect_circular.h"
#include "state/block_size_register.h"
#include "state/arch_state.h"
#include "../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_address_result valid_result(void)
{
    return ti_c54x_indirect_circular_word16(
        UINT8_C(3), UINT16_C(0x1242), UINT16_C(5),
        TI_C54X_INDIRECT_CIRCULAR_MOD10, TI_C54X_ADDRESS_ACCESS_READ);
}

static ti_c54x_address_result valid_zero_result(void)
{
    return ti_c54x_indirect_circular_word16(
        UINT8_C(3), UINT16_C(0x5502), UINT16_C(0),
        TI_C54X_INDIRECT_CIRCULAR_MOD8, TI_C54X_ADDRESS_ACCESS_WRITE);
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

static ti_c54x_arch_state known_sources(void)
{
    ti_c54x_arch_state state = known_mode();
    uint8_t index;

    for (index = UINT8_C(0); index <= UINT8_C(7); ++index) {
        const uint16_t value = index == UINT8_C(3)
                                   ? UINT16_C(0x1242)
                                   : UINT16_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                               &state, index, value) ==
                           TI_C54X_ARCH_STATE_OK);
    }
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, UINT16_C(5)) ==
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

static void test_intrinsic_shape_rejections_are_transactional(void)
{
    ti_c54x_arch_state state = known_sources();
    ti_c54x_address_result result = valid_result();

    TI_C54X_TEST_CHECK(ti_c54x_indirect_circular_effect_is_valid(
        &result.effect));
    result.effect.source.value.ar_circular.index = UINT8_C(8);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.source.value.ar_circular.modification = UINT8_C(8);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.source.value.ar_circular.bk_value_before = UINT16_C(0);
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
    result.effect.update.value_after = UINT16_C(0x1244);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    result.effect.operand.width = TI_C54X_DATA_OPERAND_WIDTH_LONG32;
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_INVALID_EFFECT);

    result = valid_result();
    expect_unchanged(&state, &result, TI_C54X_ADDRESS_UPDATE_PHASE_NONE,
                     TI_C54X_ADDRESS_APPLY_PHASE_MISMATCH);
}

static void test_dependency_order_and_unknown_stale_precedence(void)
{
    const ti_c54x_address_result result = valid_result();
    ti_c54x_arch_state state;

    /* CMPT and ARP are read before any source register. */
    state = (ti_c54x_arch_state){0};
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_MODE_STATE);
    state = known_mode();
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);

    /* A stale selected AR wins over an unknown BK because AR is read first. */
    state = known_mode();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x1243)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_STALE_SOURCE);

    /* Once AR is current, an unknown BK is reported. */
    state = known_mode();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x1242)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_UNKNOWN_SOURCE);

    /* A known but stale BK is checked only after AR is current. */
    state = known_mode();
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x1242)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, UINT16_C(6)) ==
                       TI_C54X_ARCH_STATE_OK);
    expect_unchanged(&state, &result,
                     TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS,
                     TI_C54X_ADDRESS_APPLY_STALE_SOURCE);
}

static void test_positive_success_writes_selected_ar_once(void)
{
    ti_c54x_arch_state state = known_sources();
    uint16_t before_other;
    uint16_t observed;
    uint8_t index;
    const ti_c54x_address_result result = valid_result();

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                           &state, UINT8_C(4), &before_other) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                           &state, &result,
                           TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS) ==
                       TI_C54X_ADDRESS_APPLY_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                           &state, UINT8_C(3), &observed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, UINT16_C(0x1243));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                           &state, UINT8_C(4), &observed) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, before_other);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_bk(
                           &state, &observed) == TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(observed, UINT16_C(5));
    for (index = UINT8_C(0); index <= UINT8_C(7); ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(
                               &state, index, &observed) ==
                           TI_C54X_ARCH_STATE_OK);
        if (index != UINT8_C(3) && index != UINT8_C(4)) {
            TI_C54X_TEST_CHECK_U64(observed, UINT16_C(0));
        }
    }
}

static void test_bk_zero_success_does_not_mutate(void)
{
    ti_c54x_arch_state state = known_mode();
    const ti_c54x_address_result result = valid_zero_result();

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &state, UINT8_C(3), UINT16_C(0x5502)) ==
                       TI_C54X_ARCH_STATE_OK);
    /* BK is known zero and selected AR is current for the captured effect. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&state, UINT16_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    {
        const ti_c54x_arch_state before_apply = state;
        TI_C54X_TEST_CHECK(ti_c54x_sequencer_apply_address_result(
                               &state, &result,
                               TI_C54X_ADDRESS_UPDATE_PHASE_NONE) ==
                           TI_C54X_ADDRESS_APPLY_OK);
        TI_C54X_TEST_CHECK(memcmp(&before_apply, &state, sizeof(state)) == 0);
    }
}

int main(void)
{
    test_intrinsic_shape_rejections_are_transactional();
    test_dependency_order_and_unknown_stale_precedence();
    test_positive_success_writes_selected_ar_once();
    test_bk_zero_success_does_not_mutate();
    puts("indirect_circular_effect_apply_contract: ok");
    return 0;
}
