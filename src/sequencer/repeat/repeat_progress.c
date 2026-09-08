/*
 * Module contract
 *
 * Owns: transactional, fixed-value repeat progress transitions and RC
 * validation. Does not own: CPU/PC/IRQ sequencing, timing, buses, memory,
 * opcode decode, dispatch, allocation, callbacks, observers, or logging.
 */

#include "repeat_progress.h"

#include "state/repeat_counter_register.h"

#include <stddef.h>
#include <stdint.h>

static int
phase_is_valid(ti_c54x_repeat_progress_phase phase)
{
    return phase == TI_C54X_REPEAT_PROGRESS_PHASE_IDLE ||
           phase == TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE ||
           phase == TI_C54X_REPEAT_PROGRESS_PHASE_COMPLETE;
}

ti_c54x_repeat_progress_status
ti_c54x_repeat_progress_start(const ti_c54x_op_effect *effect,
                              ti_c54x_arch_state *state,
                              ti_c54x_repeat_progress *progress)
{
    ti_c54x_arch_state staged_state;
    ti_c54x_repeat_progress staged_progress;
    uint16_t operand;

    if (effect == NULL || state == NULL || progress == NULL) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_REPEAT_START) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_EFFECT;
    }
    if (!phase_is_valid(progress->phase)) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_PHASE;
    }
    if (progress->phase == TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE) {
        uint16_t active_rc = UINT16_C(0);
        uint32_t maximum_remaining =
            (uint32_t)progress->loaded_rc_operand + UINT32_C(1);

        if (progress->body_executions_remaining == UINT32_C(0) ||
            progress->body_executions_remaining > maximum_remaining ||
            ti_c54x_arch_state_read_rc(state, &active_rc) !=
                TI_C54X_ARCH_STATE_OK ||
            active_rc != (uint16_t)(progress->body_executions_remaining -
                                    UINT32_C(1))) {
            return TI_C54X_REPEAT_PROGRESS_INVALID_STATE;
        }
        return TI_C54X_REPEAT_PROGRESS_ACTIVE;
    }
    if (progress->phase == TI_C54X_REPEAT_PROGRESS_PHASE_IDLE &&
        (progress->loaded_rc_operand != UINT16_C(0) ||
         progress->body_executions_remaining != UINT32_C(0))) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_STATE;
    }
    if (progress->phase == TI_C54X_REPEAT_PROGRESS_PHASE_COMPLETE) {
        if (progress->body_executions_remaining != UINT32_C(0)) {
            return TI_C54X_REPEAT_PROGRESS_INVALID_STATE;
        }
    }

    operand = effect->payload.repeat_start.operand;
    staged_state = *state;
    staged_progress = *progress;
    if (ti_c54x_arch_state_write_rc(&staged_state, operand) !=
        TI_C54X_ARCH_STATE_OK) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_STATE;
    }
    staged_progress.loaded_rc_operand = operand;
    staged_progress.body_executions_remaining = (uint32_t)operand + UINT32_C(1);
    staged_progress.phase = TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE;
    *state = staged_state;
    *progress = staged_progress;
    return TI_C54X_REPEAT_PROGRESS_OK;
}

ti_c54x_repeat_progress_status
ti_c54x_repeat_progress_record_body_retired(
    ti_c54x_arch_state *state,
    ti_c54x_repeat_progress *progress)
{
    uint16_t rc;
    uint32_t maximum_remaining;
    uint16_t expected_rc;

    if (state == NULL || progress == NULL) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT;
    }
    if (!phase_is_valid(progress->phase)) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_PHASE;
    }
    if (progress->phase == TI_C54X_REPEAT_PROGRESS_PHASE_COMPLETE) {
        if (progress->body_executions_remaining != UINT32_C(0)) {
            return TI_C54X_REPEAT_PROGRESS_INVALID_STATE;
        }
        return TI_C54X_REPEAT_PROGRESS_COMPLETE;
    }
    if (progress->phase != TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_STATE;
    }
    maximum_remaining = (uint32_t)progress->loaded_rc_operand + UINT32_C(1);
    if (progress->body_executions_remaining == UINT32_C(0) ||
        progress->body_executions_remaining > maximum_remaining) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_STATE;
    }
    if (ti_c54x_arch_state_read_rc(state, &rc) != TI_C54X_ARCH_STATE_OK) {
        if ((state->scalar_known_mask & TI_C54X_ARCH_STATE_KNOWN_RC) == 0U) {
            return TI_C54X_REPEAT_PROGRESS_UNKNOWN_RC;
        }
        return TI_C54X_REPEAT_PROGRESS_INVALID_STATE;
    }
    expected_rc = (uint16_t)(progress->body_executions_remaining - UINT32_C(1));
    if (rc != expected_rc) {
        return TI_C54X_REPEAT_PROGRESS_STALE_RC;
    }

    (void)ti_c54x_arch_state_write_rc(
        state,
        rc == UINT16_C(0) ? UINT16_C(0) : (uint16_t)(rc - UINT16_C(1)));
    progress->body_executions_remaining--;
    progress->phase = progress->body_executions_remaining == UINT32_C(0)
                          ? TI_C54X_REPEAT_PROGRESS_PHASE_COMPLETE
                          : TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE;
    return TI_C54X_REPEAT_PROGRESS_OK;
}
