/*
 * Module contract
 *
 * Owns: executable project-contract proof of pending-request canonical state,
 * validation, and byte-transactional rejection. Does not own: interrupt
 * sources, vectors, priority, NMI, CPU flow, timing, buses, allocation,
 * callbacks, or diagnostics.
 */

#include "sequencer/interrupt/repeat_pending_request.h"
#include "state/interrupt_flag_register.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_arch_state
reset_state(void)
{
    ti_c54x_arch_state state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static void
test_invalid_receive_is_transactional(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_pending_request pending = {0};
    ti_c54x_repeat_pending_request pending_before;
    ti_c54x_arch_state state_before;
    ti_c54x_sequencer_repeat_request_decision decision =
        TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;

    state_before = state;
    pending_before = pending;
    decision = TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(0), UINT32_C(0),
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_MASK);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);
    TI_C54X_TEST_CHECK(decision == TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(3), UINT32_C(0),
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_MASK);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(1), UINT32_C(65537),
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_COUNT);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);
    TI_C54X_TEST_CHECK(decision == TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);

    pending = (ti_c54x_repeat_pending_request){
        UINT16_C(0), (ti_c54x_repeat_pending_request_phase)UINT32_C(99)};
    pending_before = pending;
    state_before = state;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(1), UINT32_C(0),
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_PENDING);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);
    pending = (ti_c54x_repeat_pending_request){
        UINT16_C(1), TI_C54X_REPEAT_PENDING_REQUEST_EMPTY};
    pending_before = pending;
    state_before = state;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(1), UINT32_C(0),
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_PENDING);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           NULL, &pending, UINT16_C(1), UINT32_C(0),
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, NULL, UINT16_C(1), UINT32_C(0),
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(1), UINT32_C(0), NULL) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_ARGUMENT);
}

static void
test_unknown_stale_and_forged_inputs_reject(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_pending_request pending = {0};
    ti_c54x_repeat_pending_request pending_before;
    ti_c54x_arch_state state_before;
    ti_c54x_sequencer_repeat_request_decision decision =
        TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;

    state.scalar_known_mask = (uint8_t)(state.scalar_known_mask &
                                        (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_IFR);
    state_before = state;
    pending_before = pending;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(1), UINT32_C(0),
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_UNKNOWN_IFR);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);

    state = reset_state();
    pending = (ti_c54x_repeat_pending_request){0};
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(1), UINT32_C(2),
                           &decision) == TI_C54X_REPEAT_PENDING_REQUEST_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_clear_ifr_bit(
                           &state, UINT16_C(1)) == TI_C54X_ARCH_STATE_OK);
    state_before = state;
    pending_before = pending;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_on_ifr_write_one(
                           &state, &pending, UINT16_C(1)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_STALE_PENDING);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);

    state = reset_state();
    pending = (ti_c54x_repeat_pending_request){
        UINT16_C(2), TI_C54X_REPEAT_PENDING_REQUEST_LATCHED};
    pending_before = pending;
    state_before = state;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(1), UINT32_C(0),
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_PENDING);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);
    pending = (ti_c54x_repeat_pending_request){
        UINT16_C(1), TI_C54X_REPEAT_PENDING_REQUEST_LATCHED};
    pending_before = pending;
    state_before = state;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_take(
                           &state, &pending, UINT32_C(0)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_STALE_PENDING);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);

    state = reset_state();
    pending = (ti_c54x_repeat_pending_request){0};
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(1), UINT32_C(0),
                           &decision) == TI_C54X_REPEAT_PENDING_REQUEST_OK);
    pending_before = pending;
    state_before = state;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_take(
                           &state, &pending, UINT32_C(65537)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_COUNT);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);

    state.scalar_known_mask = (uint8_t)(state.scalar_known_mask &
                                        (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_IFR);
    state_before = state;
    pending_before = pending;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_take(
                           &state, &pending, UINT32_C(0)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_UNKNOWN_IFR);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_on_ifr_write_one(
                           &state, &pending, UINT16_C(1)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_UNKNOWN_IFR);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);
}

static void
test_unrelated_clear_and_reset_preserve_state(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_pending_request pending = {0};
    ti_c54x_repeat_pending_request pending_before;
    ti_c54x_arch_state state_before;
    ti_c54x_arch_state expected;
    ti_c54x_sequencer_repeat_request_decision decision =
        TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;

    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(1), UINT32_C(2),
                           &decision) == TI_C54X_REPEAT_PENDING_REQUEST_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_set_ifr_bit(
                           &state, UINT16_C(2)) == TI_C54X_ARCH_STATE_OK);
    state_before = state;
    pending_before = pending;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_on_ifr_write_one(
                           &state, &pending, UINT16_C(2)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_OK);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);
    TI_C54X_TEST_CHECK_U64(state.ifr, UINT16_C(1));
    TI_C54X_TEST_CHECK(state.pc_word == state_before.pc_word);

    state.pc_word = UINT16_C(0x4321);
    state.bk = UINT16_C(0x8765);
    state.ar[2] = UINT16_C(0x1357);
    state.status.st0 = UINT16_C(0x2468);
    state_before = state;
    expected = state;
    expected.ifr = UINT16_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_on_reset(
                           &state, &pending) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_OK);
    TI_C54X_TEST_CHECK(memcmp(&state, &expected, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(pending.phase == TI_C54X_REPEAT_PENDING_REQUEST_EMPTY);
    TI_C54X_TEST_CHECK_U64(pending.mask, UINT16_C(0));
    TI_C54X_TEST_CHECK((state.scalar_known_mask &
                        TI_C54X_ARCH_STATE_KNOWN_IFR) != 0U);
    TI_C54X_TEST_CHECK(state.pc_word == state_before.pc_word);
    TI_C54X_TEST_CHECK(state.bk == state_before.bk);
    TI_C54X_TEST_CHECK(state.ar[2] == state_before.ar[2]);
}

static void
test_null_and_empty_take_are_transactional(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_pending_request pending = {0};
    const ti_c54x_arch_state state_before = state;
    const ti_c54x_repeat_pending_request pending_before = pending;

    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_take(
                           &state, &pending, UINT32_C(0)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_NO_PENDING);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_take(
                           NULL, &pending, UINT32_C(0)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_on_ifr_write_one(
                           &state, &pending, UINT16_C(3)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_INVALID_MASK);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&pending, &pending_before, sizeof(pending)) == 0);
}

int
main(void)
{
    test_invalid_receive_is_transactional();
    test_unknown_stale_and_forged_inputs_reject();
    test_unrelated_clear_and_reset_preserve_state();
    test_null_and_empty_take_are_transactional();
    puts("repeat_pending_request_contract: ok");
    return 0;
}
