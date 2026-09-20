/*
 * Module contract
 *
 * Owns: CPU-side single/block-repeat run-loop wiring, including the
 * RPTZ same-retirement accumulator zero applied before the single
 * start. Does not own: repeat policy, PC bridge planning, stop
 * construction, retirement accounting, buses, vectors, take paths,
 * timing, observers, diagnostics, allocation, or logging.
 */

#include "repeat_activate.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../cpu_internal.h"
#include "sequencer/accumulator/effect_apply.h"
#include "sequencer/delayed_branch/delayed_branch.h"
#include "sequencer/repeat/repeat_block.h"
#include "sequencer/repeat/repeat_single.h"
#include "state/mmr_file.h"

/* An REA fall-through is pending when the block is ACTIVE, no
 * delay countdown owns the PC, the last fetch hit REA, and the
 * candidate advanced sequentially past it. REA unknownness
 * cannot pend (activation wrote it); it falls through to the
 * start path, which re-reads every cell it needs. */
static bool repeat_block_fire_pending(
    ti_c54x_cpu *cpu, ti_c54x_program_word_address candidate_pc)
{
    uint16_t rea;

    if (cpu->repeat_block_progress.phase !=
        TI_C54X_REPEAT_BLOCK_PHASE_ACTIVE) {
        return false;
    }
    if (ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch)) {
        return false;
    }
    if (ti_c54x_mmr_file_read_rea(&cpu->arch_state.mmr_file, &rea) !=
        TI_C54X_MMR_FILE_OK) {
        return false;
    }
    return cpu->last_fetch_pc_word == (ti_c54x_program_word_address)rea &&
           candidate_pc == (ti_c54x_program_word_address)rea +
                               (ti_c54x_program_word_address)
                                   cpu->last_fetch_word_count;
}

/* Retire one RPTB: an REA fall-through pending on a live block
 * completes the outer loop iteration (the RPTB start is
 * superseded, documented corner) instead of starting a nested
 * block; otherwise the driver programs RSA/REA/BRAF from the
 * copied pmad with RSA at the sequential candidate. REJECTED
 * publishes nothing. RPTBD delay-slot activation is an own
 * slice and never reaches this route. */
static ti_c54x_cpu_repeat_route_status ti_c54x_cpu_repeat_block_route(
    ti_c54x_cpu *cpu,
    const ti_c54x_op_effect *effect,
    ti_c54x_program_pc_bridge_status bridge_status,
    ti_c54x_program_word_address candidate_pc)
{
    ti_c54x_repeat_block_retired_action action =
        TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_INVALID;
    uint16_t loop_pc = UINT16_C(0);

    if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
        return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
    }
    if (ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch)) {
        return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
    }
    if (repeat_block_fire_pending(cpu, candidate_pc)) {
        if (ti_c54x_repeat_block_body_retired(
                &cpu->repeat_block_progress, &cpu->arch_state,
                cpu->last_fetch_pc_word, true, false, &action,
                &loop_pc) != TI_C54X_REPEAT_BLOCK_OK) {
            return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
        }
        if (action ==
            TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_LOOP) {
            cpu->arch_state.pc_word =
                (ti_c54x_program_word_address)loop_pc;
        } else {
            cpu->arch_state.pc_word = candidate_pc;
        }
        return TI_C54X_CPU_REPEAT_ROUTE_RETIRED;
    }
    if (candidate_pc > (ti_c54x_program_word_address)UINT16_C(0xFFFF)) {
        return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
    }
    if (ti_c54x_repeat_block_start(
            &cpu->repeat_block_progress, effect,
            (uint16_t)candidate_pc, &cpu->arch_state,
            false) != TI_C54X_REPEAT_BLOCK_OK) {
        return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
    }
    cpu->arch_state.pc_word = candidate_pc;
    return TI_C54X_CPU_REPEAT_ROUTE_RETIRED;
}

ti_c54x_cpu_repeat_route_status ti_c54x_cpu_repeat_route(
    ti_c54x_cpu *cpu,
    const ti_c54x_op_effect *effect,
    ti_c54x_program_pc_bridge_status bridge_status,
    ti_c54x_program_word_address candidate_pc)
{
    if (cpu == NULL || effect == NULL) {
        return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
    }
    if (effect->kind == TI_C54X_OP_EFFECT_REPEAT_BLOCK_START) {
        return ti_c54x_cpu_repeat_block_route(cpu, effect, bridge_status,
                                              candidate_pc);
    }
    if (effect->kind != TI_C54X_OP_EFFECT_REPEAT_START) {
        return TI_C54X_CPU_REPEAT_ROUTE_NOT_REPEAT;
    }
    if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
        return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
    }
    if (effect->payload.repeat_start.zero_destination) {
        /* RPTZ clears the destination in the same retirement that
         * loads RC: an absolute zero write with every status
         * disposition preserved (TI pledges no status bits). The
         * leaf bounds the index, but the route re-checks. */
        ti_c54x_op_effect zero_effect = {0};

        if (effect->payload.repeat_start.zero_destination_index >
            (uint8_t)TI_C54X_ACCUMULATOR_B) {
            return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
        }
        zero_effect.kind = TI_C54X_OP_EFFECT_ACCUMULATOR_WRITE;
        zero_effect.payload.accumulator_write.destination =
            effect->payload.repeat_start.zero_destination_index;
        zero_effect.payload.accumulator_write.value_raw40 = UINT64_C(0);
        zero_effect.payload.accumulator_write.status_delta.c_action =
            TI_C54X_OP_STATUS_BIT_PRESERVE;
        zero_effect.payload.accumulator_write.status_delta.tc_action =
            TI_C54X_OP_STATUS_BIT_PRESERVE;
        zero_effect.payload.accumulator_write.status_delta
            .destination_overflow_action =
            TI_C54X_OP_DESTINATION_OVERFLOW_PRESERVE;
        if (ti_c54x_accumulator_effect_apply(
                &zero_effect, &cpu->accumulators,
                &cpu->arch_state.status) != TI_C54X_ACCUMULATOR_EFFECT_OK) {
            return TI_C54X_CPU_REPEAT_ROUTE_REJECTED;
        }
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

/* Shared retirement tail for the block driver: a live block
 * only acts on an REA fall-through (last fetch hit REA with
 * sequential advance and no pending delay slots). LOOP rewinds
 * PC to RSA (HOLD: keep fetching); END leaves the sequential
 * PC with BRAF cleared; every other retirement advances with
 * no side effects. INVALID publishes nothing. */
static ti_c54x_cpu_repeat_retire_step ti_c54x_cpu_repeat_block_tail(
    ti_c54x_cpu *cpu, ti_c54x_program_word_address sequential_pc)
{
    ti_c54x_repeat_block_retired_action action =
        TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_INVALID;
    uint16_t loop_pc = UINT16_C(0);
    bool fell_through =
        sequential_pc == cpu->last_fetch_pc_word +
                             (ti_c54x_program_word_address)
                                 cpu->last_fetch_word_count;

    if (ti_c54x_repeat_block_body_retired(
            &cpu->repeat_block_progress, &cpu->arch_state,
            cpu->last_fetch_pc_word, fell_through,
            ti_c54x_sequencer_delayed_branch_is_armed(
                &cpu->delayed_branch),
            &action, &loop_pc) != TI_C54X_REPEAT_BLOCK_OK) {
        return TI_C54X_CPU_REPEAT_RETIRE_INVALID;
    }
    if (action == TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_LOOP) {
        cpu->arch_state.pc_word = (ti_c54x_program_word_address)loop_pc;
        return TI_C54X_CPU_REPEAT_RETIRE_HOLD;
    }
    return TI_C54X_CPU_REPEAT_RETIRE_ADVANCE;
}

ti_c54x_cpu_repeat_retire_step ti_c54x_cpu_repeat_body_retired(
    ti_c54x_cpu *cpu)
{
    ti_c54x_repeat_single_retired_action action =
        TI_C54X_REPEAT_SINGLE_RETIRED_ACTION_INVALID;
    ti_c54x_repeat_progress_status status;
    ti_c54x_program_word_address sequential_pc;

    if (cpu == NULL) {
        return TI_C54X_CPU_REPEAT_RETIRE_INVALID;
    }
    /* The sequential next PC, captured before single HOLD can
     * rewind it: the block check below needs the fall-through
     * candidate, not the rewound body. */
    sequential_pc = cpu->arch_state.pc_word;
    if (cpu->repeat_progress.phase !=
        TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE) {
        return ti_c54x_cpu_repeat_block_tail(cpu, sequential_pc);
    }
    status = ti_c54x_repeat_single_body_retired(
        &cpu->repeat_progress, &cpu->arch_state, &action);
    if (status == TI_C54X_REPEAT_PROGRESS_COMPLETE) {
        cpu->repeat_body_pc_word = UINT32_C(0);
        return ti_c54x_cpu_repeat_block_tail(cpu, sequential_pc);
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
        return ti_c54x_cpu_repeat_block_tail(cpu, sequential_pc);
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
