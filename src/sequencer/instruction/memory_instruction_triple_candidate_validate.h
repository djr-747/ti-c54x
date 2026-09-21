#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRIPLE_CANDIDATE_VALIDATE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRIPLE_CANDIDATE_VALIDATE_H

/*
 * Module contract
 *
 * Owns: structural and live-state validation of the copied
 * dual-data-plus-program triple arm. Structural validation
 * proves both canonical DATA READ requests, the absolute
 * PROGRAM READ request, both MOD0/MOD1/MOD2/MOD3 address
 * snapshots, and both value-owned commit descriptors against
 * the copied mode/selector/address shadows (MOD3 additionally
 * rechecks the AR0/BK shadows); live validation rechecks only
 * the planned PC and the two source AR snapshots before a
 * resume.
 * Does not own: candidate construction, transaction lifecycle, address
 * planning, CMPT/ARP policy, endpoint I/O, architectural mutation, opcode
 * semantics, timing, observers, diagnostics, or allocation. All arguments
 * are borrowed for one call and no pointer is retained.
 */

#include <stdbool.h>

#include "memory_instruction_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ti_c54x_memory_instruction_triple_snapshot_commits_match_spec(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_address_spec *spec);

bool ti_c54x_memory_instruction_triple_candidate_shape_is_valid(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_effect *effect);

bool ti_c54x_memory_instruction_triple_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_TRIPLE_CANDIDATE_VALIDATE_H */
