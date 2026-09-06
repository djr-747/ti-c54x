#ifndef TI_C54X_CPU_H
#define TI_C54X_CPU_H

/* Module contract
 * Owns: per-instance CPU state placed in caller-provided storage and the
 * lifecycle/execution API.
 * Does not own: caller storage, bus memory, bus/trace contexts, callbacks, or
 * any platform resources.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ti_c54x/bus.h>
#include <ti_c54x/state.h>
#include <ti_c54x/trace.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ti_c54x_cpu ti_c54x_cpu;

typedef enum ti_c54x_error {
    TI_C54X_ERROR_OK = 0,
    TI_C54X_ERROR_INVALID_ARGUMENT = 1,
    TI_C54X_ERROR_STORAGE_TOO_SMALL = 2,
    TI_C54X_ERROR_STORAGE_MISALIGNED = 3
} ti_c54x_error;

typedef enum ti_c54x_stop_reason {
    /* Preserve existing numeric values; append new reasons below them. */
    TI_C54X_STOP_BUDGET_EXHAUSTED = 0,
    TI_C54X_STOP_UNIMPLEMENTED_OPCODE = 1,
    TI_C54X_STOP_BUS_FAULT = 2,
    TI_C54X_STOP_IDLE = 3,
    TI_C54X_STOP_BREAKPOINT = 4,
    TI_C54X_STOP_INVALID_STATE = 5,
    /* A known instruction's public carrier is outside the common supported
     * program-word range; the fetched instruction did not retire. */
    TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS = 6,
    /* A known instruction reached the unresolved common-word boundary. */
    TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY = 7,
    /* A cumulative attempt or retirement counter could not be incremented. */
    TI_C54X_STOP_COUNTER_OVERFLOW = 8
} ti_c54x_stop_reason;

typedef struct ti_c54x_config {
    ti_c54x_program_word_address reset_pc_word;
    ti_c54x_trace_sink trace;
} ti_c54x_config;

/* ti_c54x_cpu_init copies reset_pc_word, the trace descriptor (including its
 * uint64_t event mask), and the bus descriptor.  It returns the initialized
 * opaque handle through out_cpu; callers must not cast storage to obtain it.
 * The caller retains ownership of storage, callback functions, callback
 * contexts, and memory reachable through those contexts; they must remain
 * valid through the CPU lifetime.  Reinitialization overwrites the instance
 * in place. */

typedef struct ti_c54x_run_result {
    ti_c54x_stop_reason reason;
    ti_c54x_bus_status bus_status;
    ti_c54x_program_word_address pc_word;
    uint16_t opcode_word;
    bool opcode_valid;
    /* Counts are program-instruction attempts/retirements produced by this
     * call only, not cumulative CPU totals. A failed fetch or decode attempt
     * is counted as attempted but never retired. */
    uint64_t instructions_attempted;
    uint64_t instructions_retired;
} ti_c54x_run_result;

/* The storage returned by a conforming allocator must be aligned for
 * max_align_t.  The query is provided so embedders can use an arena or a
 * caller-owned byte buffer without exposing the implementation struct. */
size_t ti_c54x_cpu_size(void);
size_t ti_c54x_cpu_alignment(void);

ti_c54x_error ti_c54x_cpu_init(void *storage,
                                size_t storage_size,
                                const ti_c54x_config *config,
                                const ti_c54x_bus *bus,
                                ti_c54x_cpu **out_cpu);
/* A non-NULL CPU handle passed to reset/inspect/step/run/deinit must have
 * come from a successful init and remain live.  NULL is accepted as a
 * deterministic invalid argument (or invalid stop result); deinit(NULL) is a
 * no-op.  Deinit invalidates the handle and does not release caller storage. */
void ti_c54x_cpu_deinit(ti_c54x_cpu *cpu);

ti_c54x_error ti_c54x_cpu_reset(ti_c54x_cpu *cpu);
ti_c54x_error ti_c54x_cpu_inspect(const ti_c54x_cpu *cpu,
                                  ti_c54x_state *state);

/* Execute at most one instruction retirement.  A step performs one complete
 * fetch/decode/dispatch/retire transaction or returns its first stop reason.
 * A failed bus fetch or unknown opcode is an attempt but not a retirement. */
ti_c54x_run_result ti_c54x_cpu_step(ti_c54x_cpu *cpu);

/* Execute until instruction_budget retirements have committed or the first
 * stop is reached.  The budget is measured in retired program instructions,
 * not bus fetches or machine cycles; attempts that fail to fetch, decode, or
 * retire are still reported in the per-call attempted count.  Successful
 * budget completion returns TI_C54X_STOP_BUDGET_EXHAUSTED and emits no stop
 * event. */
ti_c54x_run_result ti_c54x_cpu_run(ti_c54x_cpu *cpu,
                                   uint64_t instruction_budget);

#ifdef __cplusplus
}
#endif

#endif
