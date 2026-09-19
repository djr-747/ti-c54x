#ifndef TI_C54X_SEQUENCER_REPEAT_BLOCK_H
#define TI_C54X_SEQUENCER_REPEAT_BLOCK_H

/*
 * Module contract
 *
 * Owns: the block-repeat ACTIVE-lifecycle decisions that the CPU
 * borrows: guarded block start (BRAF plus RSA/REA programming
 * with BRC-knownness and delay-slot guards) and
 * body-retired LOOP versus END mapping over an REA
 * fall-through. BRC/RSA/REA stay single-truth in the MMR
 * file; this driver caches nothing. All inputs are borrowed
 * for one call; no pointer is retained and rejected calls
 * publish no partial mutation.
 * Does not own: catalog rows, decode, dispatch, MMR owner
 * storage, PC writes, CPU lifecycle, vectors, take paths, NMI
 * sources, timing, buses, observers, diagnostics, allocation,
 * or logging.
 */

#include <stdbool.h>
#include <stdint.h>

#include "isa/ops/op_effect.h"
#include "state/arch_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_repeat_block_phase {
    TI_C54X_REPEAT_BLOCK_PHASE_IDLE = 0,
    TI_C54X_REPEAT_BLOCK_PHASE_ACTIVE = 1
} ti_c54x_repeat_block_phase;

typedef enum ti_c54x_repeat_block_status {
    TI_C54X_REPEAT_BLOCK_OK = 0,
    TI_C54X_REPEAT_BLOCK_INVALID_ARGUMENT = 1,
    TI_C54X_REPEAT_BLOCK_INVALID_EFFECT = 2,
    TI_C54X_REPEAT_BLOCK_INVALID_STATE = 3,
    TI_C54X_REPEAT_BLOCK_UNKNOWN_BRC = 4
} ti_c54x_repeat_block_status;

typedef struct ti_c54x_repeat_block_progress {
    ti_c54x_repeat_block_phase phase;
} ti_c54x_repeat_block_progress;

/* Start programs RSA/REA/BRAF from a copied REPEAT_BLOCK_START
 * effect and the caller-computed block-start address. Unknown
 * BRC and delay-slot arming reject with no mutation; a nested
 * start while ACTIVE overwrites (TI-faithful reload, software
 * owns save/restore). */
ti_c54x_repeat_block_status ti_c54x_repeat_block_start(
    ti_c54x_repeat_block_progress *progress,
    const ti_c54x_op_effect *effect, uint16_t rsa,
    ti_c54x_arch_state *state, bool delayed_branch_armed);

typedef enum ti_c54x_repeat_block_retired_action {
    TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_INVALID = 0,
    TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_NONE = 1,
    TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_LOOP = 2,
    TI_C54X_REPEAT_BLOCK_RETIRED_ACTION_END = 3
} ti_c54x_repeat_block_retired_action;

/* Decide one retired instruction while ACTIVE. Only an REA
 * fall-through (retired_pc == REA with sequential advance and
 * no pending delay slots) acts: BRC > 0 decrements and loops
 * (loop_pc carries RSA), BRC == 0 ends the block (BRAF
 * cleared, phase IDLE). Every other retirement maps to NONE.
 * Unknown REA/BRC reports INVALID_STATE with no mutation. */
ti_c54x_repeat_block_status ti_c54x_repeat_block_body_retired(
    ti_c54x_repeat_block_progress *progress,
    ti_c54x_arch_state *state, uint32_t retired_pc_word,
    bool fell_through, bool delay_armed,
    ti_c54x_repeat_block_retired_action *action, uint16_t *loop_pc_word);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_REPEAT_BLOCK_H */
