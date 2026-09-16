#ifndef TI_C54X_SEQUENCER_REPEAT_SINGLE_H
#define TI_C54X_SEQUENCER_REPEAT_SINGLE_H

/*
 * Module contract
 *
 * Owns: the single-repeat ACTIVE-lifecycle decisions that the CPU
 * borrows: guarded repeat start (RC load with delay-slot and
 * nesting rejection), predecode arbitration plus body admission,
 * and body-retired HOLD versus ADVANCE mapping. Arbitration
 * affirms deferral through the request gate while executions
 * remain; the count-zero take lives with the future vector
 * slice and RS/reset preempts by construction. All inputs are
 * borrowed for one call; no pointer is retained and rejected
 * calls publish no partial mutation.
 * Does not own: catalog rows, decode, dispatch, RC/IFR owner
 * storage, PC writes, CPU lifecycle, vectors, take paths, NMI
 * sources, timing, buses, observers, diagnostics, allocation,
 * or logging.
 */

#include <stdbool.h>
#include <stdint.h>

#include "isa/catalog/opcode_catalog.h"
#include "isa/ops/op_effect.h"
#include "sequencer/interrupt/repeat_pending_request.h"
#include "sequencer/repeat/repeat_body_eligibility.h"
#include "sequencer/repeat/repeat_progress.h"
#include "state/arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Start wraps the progress reducer with the RPT route guards. All
 * progress statuses pass through unchanged; a forbidden delay-slot
 * start reports INVALID_STATE with progress and state untouched. */
ti_c54x_repeat_progress_status ti_c54x_repeat_single_start(
    ti_c54x_repeat_progress *progress,
    const ti_c54x_op_effect *effect,
    ti_c54x_arch_state *state,
    bool delayed_branch_armed);

typedef enum ti_c54x_repeat_single_predecode_status {
    TI_C54X_REPEAT_SINGLE_PREDECODE_OK = 0,
    TI_C54X_REPEAT_SINGLE_PREDECODE_INVALID_ARGUMENT = 1,
    TI_C54X_REPEAT_SINGLE_PREDECODE_INVALID_PROGRESS = 2
} ti_c54x_repeat_single_predecode_status;

/* Decide one ACTIVE body fetch before extension words issue. A
 * latched maskable request stays latched (deferral affirmed
 * through the gate, never taken here); the READY admission
 * follows the catalog class exactly. */
ti_c54x_repeat_single_predecode_status ti_c54x_repeat_single_predecode(
    const ti_c54x_repeat_progress *progress,
    ti_c54x_repeatability_class body_class,
    ti_c54x_repeat_pending_request_phase pending_phase,
    ti_c54x_sequencer_repeat_body_admission *admission);

typedef enum ti_c54x_repeat_single_retired_action {
    TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_INVALID = 0,
    TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_HOLD = 1,
    TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_ADVANCE = 2
} ti_c54x_repeat_single_retired_action;

/* Record one retired ACTIVE body. OK maps to HOLD while
 * executions remain and ADVANCE on completion; every other
 * progress status passes through with an INVALID action. */
ti_c54x_repeat_progress_status ti_c54x_repeat_single_body_retired(
    ti_c54x_repeat_progress *progress,
    ti_c54x_arch_state *state,
    ti_c54x_repeat_single_retired_action *action);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_REPEAT_SINGLE_H */
