/*
 * Module contract
 *
 * Owns: executable proof of repeat-progress admission, validation, and
 * transactional rejection. Does not own: CPU/PC/IRQ sequencing, timing,
 * buses, memory, allocation, observers, or opcode decode.
 */

#include "sequencer/repeat/repeat_progress.h"
#include "../../../../src/state/repeat_counter_register.h"
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

static ti_c54x_op_effect
repeat_effect(uint16_t operand)
{
    ti_c54x_op_effect effect = {0};

    effect.kind = TI_C54X_OP_EFFECT_REPEAT_START;
    effect.payload.repeat_start.operand = operand;
    return effect;
}

static void
test_start_rejections_are_transactional(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = {
        UINT16_C(0x1234), UINT32_C(0x5678), TI_C54X_REPEAT_PROGRESS_PHASE_IDLE
    };
    ti_c54x_arch_state state_before = state;
    ti_c54x_repeat_progress progress_before = progress;
    ti_c54x_op_effect effect = repeat_effect(UINT16_C(2));

    effect.kind = TI_C54X_OP_EFFECT_NONE;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_EFFECT);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&progress, &progress_before, sizeof(progress)) == 0);
    effect.kind = TI_C54X_OP_EFFECT_REPEAT_START;
    progress = (ti_c54x_repeat_progress){
        UINT16_C(2), UINT32_C(3), TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rc(&state, UINT16_C(2)) ==
                       TI_C54X_ARCH_STATE_OK);
    state_before = state;
    progress_before = progress;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_ACTIVE);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&progress, &progress_before, sizeof(progress)) == 0);
    progress = (ti_c54x_repeat_progress){
        UINT16_C(1), UINT32_C(0), TI_C54X_REPEAT_PROGRESS_PHASE_IDLE};
    state_before = state;
    progress_before = progress;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_STATE);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&progress, &progress_before, sizeof(progress)) == 0);
    progress.phase = (ti_c54x_repeat_progress_phase)UINT32_C(99);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_PHASE);
    progress = progress_before;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(NULL, &state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, NULL, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state, NULL) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT);
}

static void
test_start_and_record_reject_forged_or_stale_inputs(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_op_effect effect = repeat_effect(UINT16_C(1));
    ti_c54x_arch_state state_before;
    ti_c54x_repeat_progress progress_before;

    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    progress_before = progress;
    state_before = state;
    progress.body_executions_remaining = UINT32_C(0);
    progress_before = progress;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_STATE);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&progress, &progress_before, sizeof(progress)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_record_body_retired(&state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_STATE);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    progress = (ti_c54x_repeat_progress){
        UINT16_C(1), UINT32_C(2), TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE};
    progress.body_executions_remaining = UINT32_C(3);
    state_before = state;
    progress_before = progress;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_record_body_retired(&state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_STATE);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&progress, &progress_before, sizeof(progress)) == 0);
    progress = (ti_c54x_repeat_progress){
        UINT16_C(1), UINT32_C(2), TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE};
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rc(&state, UINT16_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    state_before = state;
    progress_before = progress;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_record_body_retired(&state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_STALE_RC);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&progress, &progress_before, sizeof(progress)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rc(&state, UINT16_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    state.scalar_known_mask = (uint8_t)(state.scalar_known_mask &
                                        (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_RC);
    state_before = state;
    progress_before = progress;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_record_body_retired(&state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_UNKNOWN_RC);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&progress, &progress_before, sizeof(progress)) == 0);
}

static void
test_terminal_is_distinct_and_restartable(void)
{
    ti_c54x_arch_state state = reset_state();
    ti_c54x_repeat_progress progress = {0};
    ti_c54x_op_effect effect = repeat_effect(UINT16_C(0));
    ti_c54x_arch_state state_before;
    ti_c54x_repeat_progress progress_before;

    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_record_body_retired(&state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    state_before = state;
    progress_before = progress;
    TI_C54X_TEST_CHECK(progress.phase == TI_C54X_REPEAT_PROGRESS_PHASE_COMPLETE);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_record_body_retired(&state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_COMPLETE);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&progress, &progress_before, sizeof(progress)) == 0);
    progress.body_executions_remaining = UINT32_C(1);
    state_before = state;
    progress_before = progress;
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_INVALID_STATE);
    TI_C54X_TEST_CHECK(memcmp(&state, &state_before, sizeof(state)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&progress, &progress_before, sizeof(progress)) == 0);
    progress.body_executions_remaining = UINT32_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_rc(&state, UINT16_C(0x55)) ==
                       TI_C54X_ARCH_STATE_OK);
    effect.payload.repeat_start.operand = UINT16_C(2);
    TI_C54X_TEST_CHECK(ti_c54x_repeat_progress_start(&effect, &state, &progress) ==
                       TI_C54X_REPEAT_PROGRESS_OK);
    TI_C54X_TEST_CHECK(progress.phase == TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE);
    TI_C54X_TEST_CHECK_U64(progress.loaded_rc_operand, UINT16_C(2));
    TI_C54X_TEST_CHECK_U64(state.rc, UINT16_C(2));
}

int
main(void)
{
    test_start_rejections_are_transactional();
    test_start_and_record_reject_forged_or_stale_inputs();
    test_terminal_is_distinct_and_restartable();
    puts("repeat_progress_contract: ok");
    return 0;
}
