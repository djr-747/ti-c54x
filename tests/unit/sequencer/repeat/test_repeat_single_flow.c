/*
 * Module contract
 *
 * Owns: executable proof for the single-repeat driver that composes
 * repeat start (RC load with delay-slot and nesting guards),
 * predecode (request-deferral affirmation plus body admission),
 * and body-retired (HOLD versus ADVANCE) decisions.
 * Does not own: catalog rows, decode, dispatch, RC accessors,
 * IFR sources, vectors, take paths, NMI, PC writes, CPU run
 * flow, timing, buses, callbacks, allocation, or diagnostics.
 */

#include "sequencer/repeat/repeat_single.h"
#include "sequencer/repeat/repeat_progress.h"
#include "sequencer/repeat/repeat_body_eligibility.h"
#include "sequencer/interrupt/repeat_pending_request.h"
#include "state/arch_state.h"
#include "state/interrupt_flag_register.h"
#include "state/repeat_counter_register.h"
#include "isa/catalog/opcode_catalog.h"
#include "isa/ops/op_effect.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <string.h>

static ti_c54x_arch_state
reset_state(void)
{
    ti_c54x_arch_state state;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_reset_common(&state) ==
                       TI_C54X_ARCH_STATE_OK);
    return state;
}

static ti_c54x_op_effect
repeat_start_effect(uint16_t operand)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_REPEAT_START;
    effect.payload.repeat_start.operand = operand;
    return effect;
}

static uint16_t
read_rc(const ti_c54x_arch_state *state)
{
    uint16_t rc = UINT16_C(0xBEEF);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rc(state, &rc) ==
                       TI_C54X_ARCH_STATE_OK);
    return rc;
}

static void
test_start_null_rejects(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_op_effect effect = repeat_start_effect(UINT16_C(2));

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_start(
                           NULL, &effect, &state, false) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_start(
                           &progress, NULL, &state, false) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_start(
                           &progress, &effect, NULL, false) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT);
}

static void
test_start_in_delay_slots_rejects(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_op_effect effect = repeat_start_effect(UINT16_C(2));
    const uint16_t rc_before = read_rc(&state);

    /* RPT inside delay slots is TI-forbidden: progress stays IDLE
     * and RC is untouched. */
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_start(
                           &progress, &effect, &state, true) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_STATE);
    TI_C54X_TEST_CHECK(progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_IDLE);
    TI_C54X_TEST_CHECK(progress.body_executions_remaining == UINT32_C(0));
    TI_C54X_TEST_CHECK(read_rc(&state) == rc_before);
}

static void
test_start_loads_rc(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_op_effect effect = repeat_start_effect(UINT16_C(2));

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_start(
                           &progress, &effect, &state, false) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    TI_C54X_TEST_CHECK(progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE);
    TI_C54X_TEST_CHECK(progress.loaded_rc_operand == UINT16_C(2));
    TI_C54X_TEST_CHECK(progress.body_executions_remaining == UINT32_C(3));
    TI_C54X_TEST_CHECK(read_rc(&state) == UINT16_C(2));
}

static void
test_start_nested_rejects(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_op_effect first = repeat_start_effect(UINT16_C(2));
    ti_c54x_op_effect second = repeat_start_effect(UINT16_C(5));

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_start(
                           &progress, &first, &state, false) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_start(
                           &progress, &second, &state, false) ==
                       TI_C54X_REPEAT_PROGRESS_ACTIVE);
    TI_C54X_TEST_CHECK(read_rc(&state) == UINT16_C(2));
    TI_C54X_TEST_CHECK(progress.body_executions_remaining == UINT32_C(3));
}

static void
test_start_non_repeat_effect_rejects(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_NONE;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_start(
                           &progress, &effect, &state, false) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_IDLE);
}

static void
test_predecode_null_rejects(void)
{
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_sequencer_repeat_body_admission admission =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_predecode(
                           NULL, TI_C54X_REPEATABILITY_ELIGIBLE,
                           TI_C54X_REPEAT_PENDING_REQUEST_EMPTY,
                           &admission) ==
                       TI_C54X_REPEAT_SINGLE_PREDECODE_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_predecode(
                           &progress, TI_C54X_REPEATABILITY_ELIGIBLE,
                           TI_C54X_REPEAT_PENDING_REQUEST_EMPTY,
                           NULL) ==
                       TI_C54X_REPEAT_SINGLE_PREDECODE_INVALID_ARGUMENT);
}

static void
test_predecode_idle_rejects(void)
{
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_sequencer_repeat_body_admission admission =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_predecode(
                           &progress, TI_C54X_REPEATABILITY_ELIGIBLE,
                           TI_C54X_REPEAT_PENDING_REQUEST_EMPTY,
                           &admission) ==
                       TI_C54X_REPEAT_SINGLE_PREDECODE_INVALID_PROGRESS);
}

static ti_c54x_repeat_progress
active_progress(ti_c54x_arch_state *state, uint16_t operand)
{
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_op_effect effect = repeat_start_effect(operand);

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_start(
                           &progress, &effect, state, false) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    return progress;
}

static void
test_predecode_admits_eligible(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = active_progress(&state,
                                                      UINT16_C(2));
    ti_c54x_sequencer_repeat_body_admission admission =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_BLOCK_UNRESOLVED;

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_predecode(
                           &progress, TI_C54X_REPEATABILITY_ELIGIBLE,
                           TI_C54X_REPEAT_PENDING_REQUEST_EMPTY,
                           &admission) ==
                       TI_C54X_REPEAT_SINGLE_PREDECODE_OK);
    TI_C54X_TEST_CHECK(admission ==
                       TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT);
}

static void
test_predecode_blocks_unresolved(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = active_progress(&state,
                                                      UINT16_C(2));
    ti_c54x_sequencer_repeat_body_admission admission =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_predecode(
                           &progress, TI_C54X_REPEATABILITY_UNRESOLVED,
                           TI_C54X_REPEAT_PENDING_REQUEST_EMPTY,
                           &admission) ==
                       TI_C54X_REPEAT_SINGLE_PREDECODE_OK);
    TI_C54X_TEST_CHECK(admission ==
                       TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_BLOCK_UNRESOLVED);
}

static void
test_predecode_rejects_ineligible(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = active_progress(&state,
                                                      UINT16_C(2));
    ti_c54x_sequencer_repeat_body_admission admission =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_predecode(
                           &progress, TI_C54X_REPEATABILITY_INELIGIBLE,
                           TI_C54X_REPEAT_PENDING_REQUEST_EMPTY,
                           &admission) ==
                       TI_C54X_REPEAT_SINGLE_PREDECODE_OK);
    TI_C54X_TEST_CHECK(admission ==
                       TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_REJECT_INELIGIBLE);
}

static void
test_predecode_latched_request_defers(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = active_progress(&state,
                                                      UINT16_C(2));
    ti_c54x_repeat_pending_request pending = {0};
    ti_c54x_sequencer_repeat_request_decision decision =
        TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;
    ti_c54x_sequencer_repeat_body_admission admission =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_BLOCK_UNRESOLVED;
    uint16_t ifr = UINT16_C(0);

    /* Latch a maskable request mid-repeat: the gate defers it. */
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(0x0004),
                           progress.body_executions_remaining,
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_OK);
    TI_C54X_TEST_CHECK(decision ==
                       TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_predecode(
                           &progress, TI_C54X_REPEATABILITY_ELIGIBLE,
                           pending.phase, &admission) ==
                       TI_C54X_REPEAT_SINGLE_PREDECODE_OK);
    TI_C54X_TEST_CHECK(admission ==
                       TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT);
    /* Deferral persists the latch: IFR stays set, nothing taken. */
    TI_C54X_TEST_CHECK(pending.phase ==
                       TI_C54X_REPEAT_PENDING_REQUEST_LATCHED);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ifr(&state, &ifr) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK((uint16_t)(ifr & UINT16_C(0x0004)) ==
                       UINT16_C(0x0004));
}

static void
test_predecode_latched_at_zero_rejects(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_repeat_pending_request pending = {0};
    ti_c54x_sequencer_repeat_request_decision decision =
        TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;
    ti_c54x_sequencer_repeat_body_admission admission =
        TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;

    /* A latched request with zero remaining would take, but the
     * take path lives with the future vector slice. */
    progress.phase = TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE;
    progress.loaded_rc_operand = UINT16_C(2);
    progress.body_executions_remaining = UINT32_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &state, &pending, UINT16_C(0x0004), UINT32_C(0),
                           &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_OK);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_predecode(
                           &progress, TI_C54X_REPEATABILITY_ELIGIBLE,
                           pending.phase, &admission) ==
                       TI_C54X_REPEAT_SINGLE_PREDECODE_INVALID_PROGRESS);
}

static void
test_body_retired_null_rejects(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_repeat_single_retired_action action =
        TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_HOLD;

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_body_retired(
                           NULL, &state, &action) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_body_retired(
                           &progress, NULL, &action) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_body_retired(
                           &progress, &state, NULL) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT);
}

static void
test_body_retired_lifecycle(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = active_progress(&state,
                                                      UINT16_C(2));
    ti_c54x_repeat_single_retired_action action =
        TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_INVALID;

    /* Three executions for operand 2: HOLD, HOLD, ADVANCE with RC
     * stepping 2 -> 1 -> 0 -> 0. */
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_body_retired(
                           &progress, &state, &action) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    TI_C54X_TEST_CHECK(action ==
                       TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_HOLD);
    TI_C54X_TEST_CHECK(read_rc(&state) == UINT16_C(1));
    TI_C54X_TEST_CHECK(progress.body_executions_remaining == UINT32_C(2));

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_body_retired(
                           &progress, &state, &action) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    TI_C54X_TEST_CHECK(action ==
                       TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_HOLD);
    TI_C54X_TEST_CHECK(read_rc(&state) == UINT16_C(0));
    TI_C54X_TEST_CHECK(progress.body_executions_remaining == UINT32_C(1));

    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_body_retired(
                           &progress, &state, &action) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    TI_C54X_TEST_CHECK(action ==
                       TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_ADVANCE);
    TI_C54X_TEST_CHECK(progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_COMPLETE);
    TI_C54X_TEST_CHECK(progress.body_executions_remaining == UINT32_C(0));
}

static void
test_body_retired_stale_rc_rejects(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = active_progress(&state,
                                                      UINT16_C(2));
    ti_c54x_repeat_single_retired_action action =
        TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_INVALID;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rc(&state, UINT16_C(9)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_single_body_retired(
                           &progress, &state, &action) ==
                       TI_C54X_REPEAT_PROGRESS_STALE_RC);
    TI_C54X_TEST_CHECK(action ==
                       TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_INVALID);
}

int
main(void)
{
    test_start_null_rejects();
    test_start_in_delay_slots_rejects();
    test_start_loads_rc();
    test_start_nested_rejects();
    test_start_non_repeat_effect_rejects();
    test_predecode_null_rejects();
    test_predecode_idle_rejects();
    test_predecode_admits_eligible();
    test_predecode_blocks_unresolved();
    test_predecode_rejects_ineligible();
    test_predecode_latched_request_defers();
    test_predecode_latched_at_zero_rejects();
    test_body_retired_null_rejects();
    test_body_retired_lifecycle();
    test_body_retired_stale_rc_rejects();
    return 0;
}
