/*
 * Module contract
 *
 * Owns: executable evidence proof for one maskable repeat-pending lifecycle,
 * including deferred receipt, count-zero take, and alternate clear paths.
 * Does not own: interrupt source wiring, vectors, priority, NMI, predecode,
 * PC, CPU run flow, timing, buses, callbacks, allocation, or diagnostics.
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
test_deferred_receive_then_take(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_arch_state state_before;
    ti_c54x_arch_state state_expected;
    ti_c54x_repeat_pending_request pending = {0};
    ti_c54x_sequencer_repeat_request_decision decision =
        TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;
    uint16_t ifr = UINT16_C(0);

    state.pc_word = UINT16_C(0x1234);
    state.bk = UINT16_C(0x5678);
    state.ar[3] = UINT16_C(0x9ABC);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_set_ifr_bit(
                           &state, UINT16_C(0x0001)) ==
                       TI_C54X_ARCH_STATE_OK);
    state_before = state;
    state_expected = state_before;
    state_expected.ifr = (uint16_t)(state_expected.ifr | UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(0x0004), UINT32_C(65536),
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_OK);
    TI_C54X_TEST_CHECK(decision ==
                       TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER);
    TI_C54X_TEST_CHECK(pending.phase ==
                       TI_C54X_REPEAT_PENDING_REQUEST_LATCHED);
    TI_C54X_TEST_CHECK_U64(pending.mask, UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(memcmp(&state, &state_expected, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ifr(&state, &ifr) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(ifr, UINT16_C(0x0005));
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_take(
                           &state, &pending, UINT32_C(1)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_DEFER);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_expected, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_take(
                           &state, &pending, UINT32_C(65536)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_DEFER);
    TI_C54X_TEST_CHECK(pending.phase ==
                       TI_C54X_REPEAT_PENDING_REQUEST_LATCHED);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_expected, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_take(
                           &state, &pending, UINT32_C(0)) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_OK);
    TI_C54X_TEST_CHECK(pending.phase == TI_C54X_REPEAT_PENDING_REQUEST_EMPTY);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ifr(&state, &ifr) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK_U64(ifr, UINT16_C(0x0001));
    state_expected.ifr = UINT16_C(0x0001);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_expected, sizeof(state)) == 0);
}

static void
test_normal_take_and_alternate_clear_paths(void)
{
    static const uint16_t masks[] = {
        UINT16_C(0x0001), UINT16_C(0x0002), UINT16_C(0x0008)
    };
    size_t index;

    for (index = 0U; index < sizeof(masks) / sizeof(masks[0]); ++index) {
        ti_c54x_arch_state state = reset_state();
        ti_c54x_repeat_pending_request pending = {0};
        ti_c54x_sequencer_repeat_request_decision decision =
            TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;

        TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                               &state, &pending, masks[index], UINT32_C(0),
                               &decision) ==
                           TI_C54X_REPEAT_PENDING_REQUEST_OK);
        TI_C54X_TEST_CHECK(decision ==
                           TI_C54X_SEQUENCER_REPEAT_REQUEST_NOT_BLOCKED);
        TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_take(
                               &state, &pending, UINT32_C(0)) ==
                           TI_C54X_REPEAT_PENDING_REQUEST_OK);
        TI_C54X_TEST_CHECK(pending.phase ==
                           TI_C54X_REPEAT_PENDING_REQUEST_EMPTY);
    }

    {
        ti_c54x_arch_state state = reset_state();
        ti_c54x_repeat_pending_request pending = {0};
        ti_c54x_sequencer_repeat_request_decision decision =
            TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_set_ifr_bit(
                               &state, UINT16_C(0x0001)) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                               &state, &pending, UINT16_C(0x0010), UINT32_C(2),
                               &decision) ==
                           TI_C54X_REPEAT_PENDING_REQUEST_OK);
        TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_on_ifr_write_one(
                               &state, &pending, UINT16_C(0x0010)) ==
                           TI_C54X_REPEAT_PENDING_REQUEST_OK);
        TI_C54X_TEST_CHECK(pending.phase ==
                           TI_C54X_REPEAT_PENDING_REQUEST_EMPTY);
        {
            uint16_t ifr = UINT16_C(0);

            TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ifr(&state, &ifr) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK_U64(ifr, UINT16_C(0x0001));
        }
        TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                               &state, &pending, UINT16_C(0x0020), UINT32_C(2),
                               &decision) ==
                           TI_C54X_REPEAT_PENDING_REQUEST_OK);
        TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_on_intr(
                               &state, &pending, UINT16_C(0x0020)) ==
                           TI_C54X_REPEAT_PENDING_REQUEST_OK);
        TI_C54X_TEST_CHECK(pending.phase ==
                           TI_C54X_REPEAT_PENDING_REQUEST_EMPTY);
        TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                               &state, &pending, UINT16_C(0x0040), UINT32_C(2),
                               &decision) ==
                           TI_C54X_REPEAT_PENDING_REQUEST_OK);
        TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_on_reset(
                               &state, &pending) ==
                           TI_C54X_REPEAT_PENDING_REQUEST_OK);
        TI_C54X_TEST_CHECK(pending.phase ==
                           TI_C54X_REPEAT_PENDING_REQUEST_EMPTY);
        state.scalar_known_mask = (uint8_t)(state.scalar_known_mask &
                                            (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_IFR);
        state.ifr = UINT16_C(0xFFFF);
        TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_on_reset(
                               &state, &pending) ==
                           TI_C54X_REPEAT_PENDING_REQUEST_OK);
        {
            uint16_t ifr = UINT16_C(0xFFFF);

            TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ifr(&state, &ifr) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK_U64(ifr, UINT16_C(0));
        }
    }
}

int
main(void)
{
    test_deferred_receive_then_take();
    test_normal_take_and_alternate_clear_paths();
    puts("repeat_pending_request: ok");
    return 0;
}
