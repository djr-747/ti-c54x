#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_DIRECT_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_DIRECT_H

/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied page-0 and
 * DP-relative direct root memory instruction candidates, including the
 * frozen dma/page echo, the concatenated operand, the NONE/CPL0 policy,
 * and the live CPL==0 and DP==dp9 re-reads. Also owns the shared
 * sequential-PC liveness check used by the remaining candidate arms.
 * Does not own: transaction lifecycle, endpoint I/O, architectural
 * mutation, effect completion, opcode dispatch, timing, observers, or
 * diagnostics. Candidate and state arguments are borrowed for one
 * validation call; no pointer is retained and no state is mutated.
 */

#include <stdbool.h>

#include "sequencer/program_pc_bridge.h"
#include "state/arch_state.h"

struct ti_c54x_memory_address_resolution;
struct ti_c54x_memory_instruction_candidate;
struct ti_c54x_op_memory_effect;

bool ti_c54x_memory_candidate_direct_shape_is_valid(
    const struct ti_c54x_memory_address_resolution *resolution,
    const struct ti_c54x_op_memory_effect *effect);

bool ti_c54x_memory_candidate_direct_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const ti_c54x_arch_state *state);

bool ti_c54x_memory_candidate_pc_matches_live(
    const ti_c54x_program_pc_bridge_effect *pc_effect,
    const ti_c54x_arch_state *state);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_DIRECT_H */
