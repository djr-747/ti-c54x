#ifndef TI_C54X_CPU_EXECUTION_RETIRE_PC_WRITE_H
#define TI_C54X_CPU_EXECUTION_RETIRE_PC_WRITE_H

/* Module contract
 * Owns: immediate and delayed PC-target retirement for the plain
 * (PC_WRITE, DELAYED_PC_WRITE) and far (FAR_PC_WRITE,
 * DELAYED_FAR_PC_WRITE) branch effects: target planning and
 * atomic commit, the optional S113 XPC page commit, and the
 * delay-slot countdown cancel/arm. Helpers report per-call
 * stop construction; the execution loop keeps budget
 * accounting.
 * Does not own: CPU lifetime/storage, fetch/decode
 * orchestration, the execution loop itself, opcode semantics,
 * buses, or diagnostics.
 */

#include <stdbool.h>
#include <stdint.h>

#include "../cpu_internal.h"
#include "sequencer/program_pc_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Retire one immediate PC-write effect. The far form (has_page)
 * additionally commits the copied page to XPC once the PC commit
 * is staged. Returns true with *instructions_retired advanced
 * when the instruction retired and the caller must continue;
 * returns false with *stop set when the caller must return it. */
bool ti_c54x_run_retire_pc_write(
    ti_c54x_cpu *cpu, uint16_t pmad, bool has_page, uint8_t page,
    uint32_t candidate_pc, uint8_t program_word_count,
    uint64_t instructions_attempted, uint64_t *instructions_retired,
    ti_c54x_run_result *stop);

/* Retire one delayed PC-write effect. The far form (has_page)
 * commits the copied page to XPC with the arming retirement.
 * Same return contract as ti_c54x_run_retire_pc_write. */
bool ti_c54x_run_retire_delayed_pc_write(
    ti_c54x_cpu *cpu, uint16_t pmad, bool has_page, uint8_t page,
    uint32_t candidate_pc, ti_c54x_program_pc_bridge_status bridge_status,
    uint64_t instructions_attempted, uint64_t *instructions_retired,
    ti_c54x_run_result *stop);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_CPU_EXECUTION_RETIRE_PC_WRITE_H */
