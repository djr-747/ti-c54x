/* Module contract
 * Owns: immediate and delayed PC-target retirement for the plain
 * and far branch effects, as declared in retire_pc_write.h.
 * Does not own: CPU lifetime/storage, fetch/decode orchestration,
 * the execution loop itself, opcode semantics, buses, or
 * diagnostics.
 */

#include "retire_pc_write.h"

#include <stdbool.h>
#include <stdint.h>

#include "run_support.h"
#include "sequencer/program_pc_target.h"
#include "state/program_counter_extension.h"

bool
ti_c54x_run_retire_pc_write(
    ti_c54x_cpu *cpu, uint16_t pmad, bool has_page, uint8_t page,
    uint32_t candidate_pc, uint8_t program_word_count,
    uint64_t instructions_attempted, uint64_t *instructions_retired,
    ti_c54x_run_result *stop)
{
    ti_c54x_program_pc_target_effect target_effect;
    ti_c54x_program_pc_target_status target_status;
    bool took_effect = false;

    /* PC_WRITE effects carry no status, accumulator, or T-register
     * payload, so they bypass the effect transaction and the
     * sequential bridge plan. The target planner validates the
     * absolute successor and the apply step commits it only when
     * the live carrier still matches the frozen source, which is
     * the whole atomic commit for this effect kind. The far form
     * additionally commits its copied 7-bit page through the S113
     * XPC accessor once the PC commit is staged. A PC_WRITE
     * inside delay slots is TI-forbidden; the inner redirect
     * deterministically cancels any pending countdown. */
    if (cpu == NULL || instructions_retired == NULL || stop == NULL) {
        if (stop != NULL) {
            *stop = ti_c54x_run_invalid_result();
        }
        return false;
    }
    target_status = ti_c54x_program_pc_target_plan(
        cpu->arch_state.pc_word, pmad, &target_effect);
    if (target_status != TI_C54X_PROGRAM_PC_TARGET_OK) {
        *stop = ti_c54x_run_stop_result(
            cpu, ti_c54x_run_target_status_to_stop(target_status),
            TI_C54X_BUS_OK, instructions_attempted,
            *instructions_retired);
        return false;
    }
    candidate_pc = cpu->arch_state.pc_word;
    target_status =
        ti_c54x_program_pc_target_apply(&candidate_pc, &target_effect);
    if (target_status != TI_C54X_PROGRAM_PC_TARGET_OK) {
        *stop = ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                        TI_C54X_BUS_OK,
                                        instructions_attempted,
                                        *instructions_retired);
        return false;
    }
    cpu->arch_state.pc_word = candidate_pc;
    if (has_page &&
        ti_c54x_arch_state_write_xpc(&cpu->arch_state,
                                     (uint16_t)page) !=
            TI_C54X_ARCH_STATE_OK) {
        *stop = ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                        TI_C54X_BUS_OK,
                                        instructions_attempted,
                                        *instructions_retired);
        return false;
    }
    cpu->instructions_retired++;
    (*instructions_retired)++;
    if (ti_c54x_run_advance_delayed_branch(cpu, program_word_count, true,
                                           &took_effect) !=
        TI_C54X_SEQUENCER_DELAYED_BRANCH_OK) {
        *stop = ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                        TI_C54X_BUS_OK,
                                        instructions_attempted,
                                        *instructions_retired);
        return false;
    }
    return true;
}

bool
ti_c54x_run_retire_delayed_pc_write(
    ti_c54x_cpu *cpu, uint16_t pmad, bool has_page, uint8_t page,
    uint32_t candidate_pc, ti_c54x_program_pc_bridge_status bridge_status,
    uint64_t instructions_attempted, uint64_t *instructions_retired,
    ti_c54x_run_result *stop)
{
    ti_c54x_program_pc_target_effect target_effect;
    ti_c54x_program_pc_target_status target_status;

    /* Delayed PC writes retire with sequential advance and arm the
     * CPU-owned countdown with the copied pmad, replacing any
     * pending countdown under the last-wins rule; the arming
     * instruction consumes no delay-slot words itself. The far
     * form commits its copied page through the S113 XPC accessor
     * with the arming retirement: only the six far control
     * instructions modify XPC, so no instruction inside the two
     * delay slots observes the page early except a TI-forbidden
     * inner branch, which deterministically replaces both the
     * countdown and XPC under the same last-wins rule. The target
     * planner validates the absolute successor before any commit;
     * a full 16-bit pmad is always encodable, so planning fails
     * only on caller-argument defects. A delayed branch inside
     * delay slots is TI-forbidden; the inner arming wins
     * deterministically here. No MEMORY endpoint, no WAIT/FAULT,
     * and no status/accumulator/T effect is admitted. */
    if (cpu == NULL || instructions_retired == NULL || stop == NULL) {
        if (stop != NULL) {
            *stop = ti_c54x_run_invalid_result();
        }
        return false;
    }
    target_status = ti_c54x_program_pc_target_plan(
        cpu->arch_state.pc_word, pmad, &target_effect);
    if (target_status != TI_C54X_PROGRAM_PC_TARGET_OK) {
        *stop = ti_c54x_run_stop_result(
            cpu, ti_c54x_run_target_status_to_stop(target_status),
            TI_C54X_BUS_OK, instructions_attempted,
            *instructions_retired);
        return false;
    }
    if (bridge_status != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
        *stop = ti_c54x_run_stop_result(
            cpu, ti_c54x_run_bridge_status_to_stop(bridge_status),
            TI_C54X_BUS_OK, instructions_attempted,
            *instructions_retired);
        return false;
    }
    cpu->arch_state.pc_word = candidate_pc;
    if (has_page &&
        ti_c54x_arch_state_write_xpc(&cpu->arch_state,
                                     (uint16_t)page) !=
            TI_C54X_ARCH_STATE_OK) {
        *stop = ti_c54x_run_stop_result(cpu, TI_C54X_STOP_INVALID_STATE,
                                        TI_C54X_BUS_OK,
                                        instructions_attempted,
                                        *instructions_retired);
        return false;
    }
    cpu->instructions_retired++;
    (*instructions_retired)++;
    (void)ti_c54x_sequencer_delayed_branch_arm(&cpu->delayed_branch,
                                               pmad);
    return true;
}
