/*
 * Module contract
 *
 * Owns: the single-repeat ACTIVE-lifecycle decisions that the CPU
 * borrows. Does not own: catalog rows, decode, dispatch, RC/IFR
 * owner storage, PC writes, CPU lifecycle, vectors, take paths,
 * NMI sources, timing, buses, observers, diagnostics,
 * allocation, or logging.
 */

#include "repeat_single.h"

#include <stddef.h>
#include <stdint.h>

#include "sequencer/repeat/repeat_request_gate.h"

ti_c54x_repeat_progress_status ti_c54x_repeat_single_start(
    ti_c54x_repeat_progress *progress,
    const ti_c54x_op_effect *effect,
    ti_c54x_arch_state *state,
    bool delayed_branch_armed)
{
    if (progress == NULL || effect == NULL || state == NULL) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT;
    }
    /* RPT inside delay slots is TI-forbidden: reject before the
     * progress reducer can stage anything. */
    if (delayed_branch_armed) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_STATE;
    }
    return ti_c54x_repeat_progress_start(effect, state, progress);
}

ti_c54x_repeat_single_predecode_status ti_c54x_repeat_single_predecode(
    const ti_c54x_repeat_progress *progress,
    ti_c54x_repeatability_class body_class,
    ti_c54x_repeat_pending_request_phase pending_phase,
    ti_c54x_sequencer_repeat_body_admission *admission)
{
    ti_c54x_sequencer_repeat_body_admission staged;

    if (progress == NULL || admission == NULL) {
        return TI_C54X_REPEAT_SINGLE_PREDECODE_INVALID_ARGUMENT;
    }
    if (progress->phase != TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE) {
        return TI_C54X_REPEAT_SINGLE_PREDECODE_INVALID_PROGRESS;
    }
    if (pending_phase != TI_C54X_REPEAT_PENDING_REQUEST_EMPTY &&
        pending_phase != TI_C54X_REPEAT_PENDING_REQUEST_LATCHED) {
        return TI_C54X_REPEAT_SINGLE_PREDECODE_INVALID_ARGUMENT;
    }
    if (pending_phase == TI_C54X_REPEAT_PENDING_REQUEST_LATCHED) {
        ti_c54x_sequencer_repeat_request_decision decision =
            TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;

        /* The arbitration phase binds here: a latched maskable
         * request consults the gate and stays latched. The gate
         * admits only at zero remaining, but zero remaining never
         * takes here — the count-zero take lives with the future
         * vector slice. */
        if (ti_c54x_sequencer_repeat_request_gate_evaluate(
                progress->body_executions_remaining,
                TI_C54X_SEQUENCER_REPEAT_REQUEST_MASKABLE_INTERRUPT,
                &decision) !=
                TI_C54X_SEQUENCER_REPEAT_REQUEST_GATE_OK ||
            decision != TI_C54X_SEQUENCER_REPEAT_REQUEST_DEFER) {
            return TI_C54X_REPEAT_SINGLE_PREDECODE_INVALID_PROGRESS;
        }
    }
    if (ti_c54x_sequencer_repeat_body_admission_from_class(
            body_class, &staged) !=
        TI_C54X_SEQUENCER_REPEAT_BODY_ELIGIBILITY_OK) {
        return TI_C54X_REPEAT_SINGLE_PREDECODE_INVALID_ARGUMENT;
    }
    *admission = staged;
    return TI_C54X_REPEAT_SINGLE_PREDECODE_OK;
}

ti_c54x_repeat_progress_status ti_c54x_repeat_single_body_retired(
    ti_c54x_repeat_progress *progress,
    ti_c54x_arch_state *state,
    ti_c54x_repeat_single_retired_action *action)
{
    ti_c54x_repeat_progress_status status;

    if (progress == NULL || state == NULL || action == NULL) {
        return TI_C54X_REPEAT_PROGRESS_INVALID_ARGUMENT;
    }
    status = ti_c54x_repeat_progress_record_body_retired(state, progress);
    if (status == TI_C54X_REPEAT_PROGRESS_OK) {
        *action = progress->phase ==
                          TI_C54X_REPEAT_PROGRESS_PHASE_COMPLETE
                      ? TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_ADVANCE
                      : TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_HOLD;
        return TI_C54X_REPEAT_PROGRESS_OK;
    }
    if (status == TI_C54X_REPEAT_PROGRESS_COMPLETE) {
        *action = TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_ADVANCE;
        return TI_C54X_REPEAT_PROGRESS_COMPLETE;
    }
    *action = TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_INVALID;
    return status;
}
