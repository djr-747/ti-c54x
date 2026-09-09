#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_CANDIDATE_VALIDATE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_CANDIDATE_VALIDATE_H

/*
 * Module contract
 *
 * Owns: structural and live-state validation of the copied W50A/W51B
 * dual-memory candidate arm. Structural validation proves both canonical
 * DATA READ requests, both MOD0/MOD1/MOD2 address snapshots, and both
 * value-owned commit descriptors against the copied mode/selector/address
 * shadows; live validation rechecks only the planned PC and the two source
 * AR snapshots before a resume.
 * Does not own: candidate construction, transaction lifecycle, address
 * planning, CMPT/ARP policy, endpoint I/O, architectural mutation, opcode
 * semantics, timing, observers, diagnostics, or allocation. All arguments
 * are borrowed for one call and no pointer is retained.
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

/* Prove both snapshot commits equal the planner contract for the copied
 * mode/selector/address shadows: MOD0 and aliased-Y sides hold a zeroed
 * no-update commit, MOD1/MOD2 sides hold the exact non-wrapping post-access
 * decrement/increment of the frozen request address, and MOD3 sides hold
 * the exact AR0/BK post-access update recomputed from the AR0/BK shadows
 * (or a zeroed no-update commit for the BK=0/AR0=0 case). */
bool ti_c54x_memory_instruction_dual_snapshot_commits_match_spec(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_address_spec *spec);

bool ti_c54x_memory_instruction_dual_candidate_shape_is_valid(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_effect *effect);

bool ti_c54x_memory_instruction_dual_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_DUAL_CANDIDATE_VALIDATE_H */
