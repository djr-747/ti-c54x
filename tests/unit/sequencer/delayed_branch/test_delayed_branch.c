/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proofs for the pure
 * delayed-branch countdown value. It checks disarmed init, arm with
 * a two-word countdown and replace-on-arm, single- and double-word
 * consume with take-effect reporting, saturating consume for a slot
 * fill wider than the remaining words, disarm, and strict rejection
 * of disarmed, zero-width, and null calls.
 * Does not own: address planning, bus transactions, CPU lifecycle,
 * opcode dispatch, observers, or diagnostics. The caller owns the
 * output.
 */

#include "sequencer/delayed_branch/delayed_branch.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stdint.h>

static void
test_init_disarms(void)
{
    ti_c54x_sequencer_delayed_branch_state state = {
        true, UINT8_C(7), UINT16_C(0x1234)};
    bool took_effect = false;

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_init(&state) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(!ti_c54x_sequencer_delayed_branch_is_armed(&state));
    TI_C54X_TEST_CHECK(state.words_remaining == UINT8_C(0));
    TI_C54X_TEST_CHECK(state.deferred_pc_word == UINT16_C(0));
    /* Rejected calls leave caller outputs untouched. */
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_consume(
                           &state, UINT8_C(1), &took_effect) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(!took_effect);
}

static void
test_arm_starts_two_word_countdown(void)
{
    ti_c54x_sequencer_delayed_branch_state state = {0};
    bool took_effect = true;

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_arm(
                           &state, UINT16_C(0x1000)) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_is_armed(&state));
    TI_C54X_TEST_CHECK(state.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(state.deferred_pc_word == UINT16_C(0x1000));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_consume(
                           &state, UINT8_C(1), &took_effect) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(!took_effect);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_is_armed(&state));
    TI_C54X_TEST_CHECK(state.words_remaining == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_consume(
                           &state, UINT8_C(1), &took_effect) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(took_effect);
    TI_C54X_TEST_CHECK(!ti_c54x_sequencer_delayed_branch_is_armed(&state));
    TI_C54X_TEST_CHECK(state.deferred_pc_word == UINT16_C(0x1000));
}

static void
test_two_word_instruction_takes_effect_at_once(void)
{
    ti_c54x_sequencer_delayed_branch_state state = {0};
    bool took_effect = false;

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_arm(
                           &state, UINT16_C(0x2000)) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_consume(
                           &state, UINT8_C(2), &took_effect) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(took_effect);
    TI_C54X_TEST_CHECK(!ti_c54x_sequencer_delayed_branch_is_armed(&state));
}

static void
test_oversize_consume_still_takes_effect(void)
{
    /* Malformed slot fill (one word left, a two-word instruction
     * retires) gets a total rule: the branch takes effect rather
     * than halting. */
    ti_c54x_sequencer_delayed_branch_state state = {0};
    bool took_effect = false;

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_arm(
                           &state, UINT16_C(0x00a5)) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_consume(
                           &state, UINT8_C(1), &took_effect) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(!took_effect);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_consume(
                           &state, UINT8_C(2), &took_effect) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(took_effect);
    TI_C54X_TEST_CHECK(!ti_c54x_sequencer_delayed_branch_is_armed(&state));
    TI_C54X_TEST_CHECK(state.deferred_pc_word == UINT16_C(0x00a5));
}

static void
test_arm_replaces_pending_branch(void)
{
    ti_c54x_sequencer_delayed_branch_state state = {0};
    bool took_effect = true;

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_arm(
                           &state, UINT16_C(0x1000)) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_consume(
                           &state, UINT8_C(1), &took_effect) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(!took_effect);
    /* A delayed branch inside a delay slot replaces the outer one;
     * the arming instruction consumes no outer slot words. */
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_arm(
                           &state, UINT16_C(0x3000)) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(state.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(state.deferred_pc_word == UINT16_C(0x3000));
}

static void
test_disarm_clears(void)
{
    ti_c54x_sequencer_delayed_branch_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_arm(
                           &state, UINT16_C(0x1000)) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_disarm(&state) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(!ti_c54x_sequencer_delayed_branch_is_armed(&state));
    TI_C54X_TEST_CHECK(state.words_remaining == UINT8_C(0));
    TI_C54X_TEST_CHECK(state.deferred_pc_word == UINT16_C(0));
}

static void
test_strict_rejections(void)
{
    ti_c54x_sequencer_delayed_branch_state state = {0};
    bool took_effect = false;

    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_init(NULL) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_arm(NULL,
                                                            UINT16_C(0)) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_disarm(NULL) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(!ti_c54x_sequencer_delayed_branch_is_armed(NULL));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_consume(
                           NULL, UINT8_C(1), &took_effect) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_consume(
                           &state, UINT8_C(1), NULL) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT);
    /* Disarmed consume and zero-width consume are programmer errors. */
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_consume(
                           &state, UINT8_C(1), &took_effect) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_arm(
                           &state, UINT16_C(0x1000)) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_consume(
                           &state, UINT8_C(0), &took_effect) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(!took_effect);
    TI_C54X_TEST_CHECK(state.words_remaining == UINT8_C(2));
}

static void
test_zeroed_state_reads_disarmed(void)
{
    /* CPU init clears storage with memset: zero must read disarmed. */
    ti_c54x_sequencer_delayed_branch_state state = {0};

    TI_C54X_TEST_CHECK(!ti_c54x_sequencer_delayed_branch_is_armed(&state));
    TI_C54X_TEST_CHECK(state.words_remaining == UINT8_C(0));
    TI_C54X_TEST_CHECK(state.deferred_pc_word == UINT16_C(0));
}

int
main(void)
{
    test_zeroed_state_reads_disarmed();
    test_init_disarms();
    test_arm_starts_two_word_countdown();
    test_two_word_instruction_takes_effect_at_once();
    test_oversize_consume_still_takes_effect();
    test_arm_replaces_pending_branch();
    test_disarm_clears();
    test_strict_rejections();
    puts("delayed_branch: ok");
    return 0;
}
