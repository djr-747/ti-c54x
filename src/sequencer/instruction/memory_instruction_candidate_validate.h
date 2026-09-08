#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_CANDIDATE_VALIDATE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_CANDIDATE_VALIDATE_H

/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied root memory
 * instruction candidates, including exact address-resolution snapshots for
 * single-indirect, AR0-indexed, and circular operands.
 * Does not own: transaction lifecycle, endpoint I/O, architectural mutation,
 * effect completion, opcode dispatch, timing, observers, or diagnostics.
 * Candidate and state arguments are borrowed for one validation call; no
 * pointer is retained and no state is mutated.
 */

#include <stdbool.h>

#include "state/arch_state.h"

struct ti_c54x_memory_instruction_candidate;

bool ti_c54x_memory_instruction_candidate_shape_is_valid(
    const struct ti_c54x_memory_instruction_candidate *candidate);

bool ti_c54x_memory_instruction_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const ti_c54x_arch_state *state);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_CANDIDATE_VALIDATE_H */
