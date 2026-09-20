#ifndef TI_C54X_CPU_EXECUTION_TAKE_ISSUE_H
#define TI_C54X_CPU_EXECUTION_TAKE_ISSUE_H

/*
 * Module contract
 *
 * Owns: NMI take issue for the step/run loop: predecode
 * arbitration (a pending edge preempts fetch at the
 * instruction boundary unless a repeat loop or delay slot
 * holds it), take-effect construction, root prepare/resume
 * through the take producer, retirement accounting with
 * edge clearing, and the shared retirement tail. Deferred
 * edges return NOT_ARMED so the ordinary fetch proceeds; a
 * retired take returns CONTINUE so the loop re-checks its
 * budget; WAIT and failures fill the stop the caller
 * constructs with the shared constructors.
 * Does not own: CPU lifetime/storage, fetch/decode
 * orchestration, the execution loop itself, stop/wait
 * construction, arbitration policy, take preparation or
 * commit, buses, timing, observers, or diagnostics. The CPU
 * and counter out-parameters are borrowed for one call.
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti_c54x/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_cpu_take_issue_outcome {
    TI_C54X_CPU_TAKE_ISSUE_NOT_ARMED = 0,
    TI_C54X_CPU_TAKE_ISSUE_CONTINUE = 1,
    TI_C54X_CPU_TAKE_ISSUE_WAIT = 2,
    TI_C54X_CPU_TAKE_ISSUE_STOP = 3
} ti_c54x_cpu_take_issue_outcome;

typedef struct ti_c54x_cpu_take_issue_result {
    ti_c54x_cpu_take_issue_outcome outcome;
    ti_c54x_stop_reason stop_reason;
    ti_c54x_bus_status bus_status;
} ti_c54x_cpu_take_issue_result;

ti_c54x_cpu_take_issue_result ti_c54x_cpu_take_issue(
    ti_c54x_cpu *cpu, uint64_t *instructions_attempted,
    uint64_t *instructions_retired);

/* Clear the latched NMI edge after a resumed root retires a take
 * candidate. WAIT and FAULT preserve the edge for resume or
 * retry; opcode candidates never trigger the clear. */
void ti_c54x_cpu_take_note_resumed(ti_c54x_cpu *cpu);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_CPU_EXECUTION_TAKE_ISSUE_H */
