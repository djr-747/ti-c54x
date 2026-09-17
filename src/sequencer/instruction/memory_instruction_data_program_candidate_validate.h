#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DATA_PROGRAM_CANDIDATE_VALIDATE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DATA_PROGRAM_CANDIDATE_VALIDATE_H

/*
 * Module contract
 *
 * Owns: structural and live-state validation of the copied
 * DATA+PROGRAM pair candidate arm. Structural validation proves the
 * canonical DATA READ request, the absolute PROGRAM READ request,
 * the single Smem address snapshot, and both value-owned commit
 * descriptors against the copied leg-selector/address shadows (the
 * PROGRAM side always holds a zeroed no-update commit); live
 * validation rechecks only the planned PC and the Smem source
 * snapshot before a resume.
 * Does not own: candidate construction, transaction lifecycle,
 * address planning, CMPT/ARP policy, endpoint I/O, architectural
 * mutation, opcode semantics, timing, observers, diagnostics, or
 * allocation. All arguments are borrowed for one call and no
 * pointer is retained.
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

/* Prove both snapshot commits equal the planner contract for the
 * copied leg shadows: direct and MOD0 Smem sides hold a zeroed
 * no-update commit, MOD1/MOD2 sides hold the exact non-wrapping
 * post-access decrement/increment of the frozen DATA request
 * address, and the PROGRAM side always holds a zeroed no-update
 * commit. */
bool ti_c54x_memory_instruction_data_program_snapshot_commits_match_spec(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_address_spec *spec);

bool ti_c54x_memory_instruction_data_program_candidate_shape_is_valid(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_effect *effect);

bool ti_c54x_memory_instruction_data_program_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DATA_PROGRAM_CANDIDATE_VALIDATE_H */
