#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_PORT_CANDIDATE_VALIDATE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_PORT_CANDIDATE_VALIDATE_H

/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied PORT pair
 * candidates: the DATA-leg shape recheck (DATA-spec synthesis plus
 * single-leg resolution validation plus IO-leg arm binding) and
 * direct-DP DATA-leg liveness. Indirect-family DATA legs reuse the
 * shared single-leg tail in the root validator.
 * Does not own: transaction lifecycle, endpoint I/O, architectural
 * mutation, effect completion, opcode dispatch, timing, observers,
 * or diagnostics. Candidate and state arguments are borrowed for
 * one validation call; no pointer is retained and no state is
 * mutated.
 */

#include <stdbool.h>

struct ti_c54x_memory_address_resolution;
struct ti_c54x_memory_instruction_candidate;
struct ti_c54x_arch_state;

bool ti_c54x_memory_instruction_port_candidate_shape_is_valid(
    const struct ti_c54x_memory_instruction_candidate *candidate);

bool ti_c54x_memory_instruction_port_direct_data_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state,
    const struct ti_c54x_memory_address_resolution *resolution);

/* PORT absolute-Smem DATA-leg liveness (P3-S85). The absolute leg
 * observes no state: re-prove the frozen absolute echo against the
 * marker's DATA address and pin the sequential PC. */
bool ti_c54x_memory_instruction_port_absolute_data_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state,
    const struct ti_c54x_memory_address_resolution *resolution);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_PORT_CANDIDATE_VALIDATE_H */
