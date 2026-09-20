#ifndef TI_C54X_CPU_EXECUTION_RUN_SUPPORT_H
#define TI_C54X_CPU_EXECUTION_RUN_SUPPORT_H

/* Module contract
 * Owns: run-loop result construction, stop/wait mapping, the
 * deferred-branch adopt/advance/apply plumbing, and the shared
 * memory-root retirement tail (adopt, countdown advance, root
 * reset, repeat re-drive) used by resumed roots, opcode MEMORY
 * retirements, and interrupt takes. Pure helpers over explicit
 * arguments; only the stop/wait constructors clear the
 * execution-active flag.
 * Does not own: CPU lifetime/storage, fetch/decode orchestration,
 * the execution loop itself, bus descriptor validation, lifecycle
 * cancellation, trace assembly, or opcode semantics.
 */

#include <stdbool.h>
#include <stdint.h>

#include "../cpu_internal.h"
#include "../trace/emit.h"
#include "isa/ops/op_effect.h"
#include "sequencer/delayed_branch/delayed_branch.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/program_pc_bridge.h"
#include "sequencer/program_pc_target.h"

#ifdef __cplusplus
extern "C" {
#endif

ti_c54x_run_result ti_c54x_run_result_from_cpu(
    const ti_c54x_cpu *cpu, ti_c54x_stop_reason reason,
    ti_c54x_bus_status bus_status, uint64_t instructions_attempted,
    uint64_t instructions_retired);

ti_c54x_run_result ti_c54x_run_invalid_result(void);

ti_c54x_run_result ti_c54x_run_stop_result(
    ti_c54x_cpu *cpu, ti_c54x_stop_reason reason,
    ti_c54x_bus_status bus_status, uint64_t instructions_attempted,
    uint64_t instructions_retired);

ti_c54x_run_result ti_c54x_run_wait_result(
    ti_c54x_cpu *cpu, uint64_t instructions_attempted,
    uint64_t instructions_retired);

ti_c54x_stop_reason ti_c54x_run_bridge_status_to_stop(
    ti_c54x_program_pc_bridge_status status);

bool ti_c54x_run_memory_status_ok(
    ti_c54x_memory_instruction_status status);

ti_c54x_stop_reason ti_c54x_run_target_status_to_stop(
    ti_c54x_program_pc_target_status status);

void ti_c54x_run_adopt_deferred_branch(
    ti_c54x_cpu *cpu, const ti_c54x_memory_instruction_result *result);

bool ti_c54x_run_memory_effect_redirects_pc(
    const ti_c54x_op_effect *effect);

ti_c54x_sequencer_delayed_branch_status ti_c54x_run_advance_delayed_branch(
    ti_c54x_cpu *cpu, uint8_t word_count, bool redirected_pc,
    bool *took_effect);

void ti_c54x_run_apply_deferred_branch(ti_c54x_cpu *cpu);

/* Shared retirement tail for one RETIRED memory-root work unit:
 * adopt any deferred-branch report, advance the countdown unless
 * the retirement armed one (a retired redirect applies on
 * take-effect), reset the root, and re-drive the repeat HOLD
 * tail. Every failure maps to INVALID_STATE, matching each
 * caller's previous inline mapping; true means the caller
 * continues its loop. */
bool ti_c54x_run_memory_retirement_tail(
    ti_c54x_cpu *cpu,
    const ti_c54x_memory_instruction_result *result,
    uint8_t retired_word_count,
    bool retired_redirects_pc);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_CPU_EXECUTION_RUN_SUPPORT_H */
