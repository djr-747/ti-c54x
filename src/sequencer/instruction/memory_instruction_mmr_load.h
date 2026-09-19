#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_MMR_LOAD_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_MMR_LOAD_H

/*
 * Module contract
 *
 * Owns: the read-phase MMR load snapshot (MVMD) used by the root
 * memory transaction. It recognizes an armed absolute-DATA WRITE
 * with a LOAD sidecar leg, snapshots the named MMR through the
 * router, and patches the prepared DATA WRITE request before
 * first issue; WAIT retries re-issue the same request. Decline
 * or unknown MMR reads fail closed ahead of any endpoint
 * traffic. The staged effect value stays zero (the gate rejects
 * forged nonzero values), so the snapshot never touches the
 * effect. Transaction storage is borrowed for each call.
 * Does not own: carrier validity, address resolution, candidate
 * shape or liveness, the issue orchestrator itself, CPU
 * publication, endpoint callbacks, timing, observers, or
 * diagnostics.
 */

#include <stdbool.h>

#include "memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Snapshot the LOAD-leg MMR into the prepared DATA WRITE
 * request. Non-LOAD transactions pass through untouched;
 * defensive mismatches (non-WRITE access, nonzero staged
 * value) and router decline/unknown return false. */
bool ti_c54x_memory_instruction_snapshot_mmr_load(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_MMR_LOAD_H */
