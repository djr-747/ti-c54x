/*
 * Module contract
 *
 * Owns: CPU-side single-repeat run-loop wiring. Does not own:
 * repeat policy, PC bridge planning, stop construction,
 * retirement accounting, buses, vectors, take paths, timing,
 * observers, diagnostics, allocation, or logging.
 */

#include "repeat_activate.h"

#include <stddef.h>

#include "../cpu_internal.h"
#include "sequencer/delayed_branch/delayed_branch.h"
#include "sequencer/repeat/repeat_single.h"

ti_c54x_cpu_repeat_route_status ti_c54x_cpu_repeat_route(
    ti_c54x_cpu *cpu,
    const ti_c54x_op_effect *effect,
    ti_c54x_program_pc_bridge_status bridge_status,
    ti_c54x_program_word_address candidate_pc)
{
    if (cpu == NULL || effect == NULL) {
        return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_REPEAT_START) {
        return TI_C54X_CPU_REPEAT_ROUTE_NOT_REPEAT;
    }
    if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
        return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
    }
    if (ti_c54x_repeat_single_start(
            &cpu->repeat_progress, effect, &cpu->arch_state,
            ti_c54x_sequencer_delayed_branch_is_armed(
                &cpu->delayed_branch)) != TI_C54X_REPEAT_PROGRESS_OK) {
        return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
    }
    cpu->arch_state.pc_word = candidate_pc;
    cpu->repeat_body_pc_word = candidate_pc;
    return TI_C54X_CPU_REPEAT_ROUTE_RETIRED;
}

ti_c54x_cpu_repeat_retire_step ti_c54x_cpu_repeat_body_retired(
    ti_c54x_cpu *cpu)
{
    ti_c54x_repeat_single_retired_action action =
        TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_INVALID;
    ti_c54x_repeat_progress_status status;

    if (cpu == NULL) {
        return TI_C54X_CPU_REPEAT_RETIRE_INVALID;
    }
    if (cpu->repeat_progress.phase !=
        TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE) {
        return TI_C54X_CPU_REPEAT_RETIRE_ADVANCE;
    }
    status = ti_c54x_repeat_single_body_retired(
        &cpu->repeat_progress, &cpu->arch_state, &action);
    if (status == TI_C54X_REPEAT_PROGRESS_COMPLETE) {
        cpu->repeat_body_pc_word = UINT32_C(0);
        return TI_C54X_CPU_REPEAT_RETIRE_ADVANCE;
    }
    if (status != TI_C54X_REPEAT_PROGRESS_OK) {
        return TI_C54X_CPU_REPEAT_RETIRE_INVALID;
    }
    if (action == TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_HOLD) {
        cpu->arch_state.pc_word = cpu->repeat_body_pc_word;
        return TI_C54X_CPU_REPEAT_RETIRE_HOLD;
    }
    if (action == TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_ADVANCE) {
        cpu->repeat_body_pc_word = UINT32_C(0);
        return TI_C54X_CPU_REPEAT_RETIRE_ADVANCE;
    }
    return TI_C54X_CPU_REPEAT_RETIRE_INVALID;
}

bool ti_c54x_cpu_repeat_predecode_blocks(
    ti_c54x_cpu *cpu,
    ti_c54x_repeatability_class body_class)
{
    ti_c54x_sequencer_repeat_body_admission admission;

    if (cpu == NULL) {
        return true;
    }
    if (cpu->repeat_progress.phase !=
        TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE) {
        return false;
    }
    return ti_c54x_repeat_single_predecode(
               &cpu->repeat_progress, body_class,
               cpu->repeat_pending.phase, &admission) !=
               TI_C54X_REPEAT_SINGLE_PREDECODE_OK ||
           admission != TI_C54X_SEQUENCER_REPEAT_BODY_ADMISSION_ADMIT;
}
