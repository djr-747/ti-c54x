#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_MOVE_CANDIDATE_VALIDATE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_MOVE_CANDIDATE_VALIDATE_H

/*
 * Module contract
 *
 * Owns: structural and live-state validation of the copied
 * dual-move candidate arm. Structural validation proves the
 * move-pair kind, WRITE access with null completions, the
 * X READ and Y WRITE DATA requests, both MOD0/MOD1/MOD2/MOD3
 * address snapshots, and both value-owned commit descriptors
 * against the copied mode/selector/address shadows (MOD3
 * additionally rechecks the AR0/BK shadows via the shared
 * dual commit matcher); live validation rechecks only the
 * planned PC and the two source AR snapshots before a resume.
 * Does not own: candidate construction, transaction lifecycle,
 * address planning, CMPT/ARP policy, endpoint I/O,
 * architectural mutation, opcode semantics, timing, observers,
 * diagnostics, or allocation. All arguments are borrowed for
 * one call and no pointer is retained.
 */

#include <stdbool.h>

#include "sequencer/memory/dual_address_resolution.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ti_c54x_memory_instruction_candidate;
struct ti_c54x_op_memory_effect;
struct ti_c54x_op_memory_address_spec;
struct ti_c54x_arch_state;

bool ti_c54x_memory_instruction_dual_move_candidate_shape_is_valid(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_effect *effect);

bool ti_c54x_memory_instruction_dual_move_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_MOVE_CANDIDATE_VALIDATE_H */
