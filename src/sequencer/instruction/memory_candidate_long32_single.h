/* Module contract
 *
 * Owns: structural and live-state validation of one copied
 * LONG32_SINGLE DATA address resolution against its memory
 * effect: the canonical MSW/LSW pairing with the page-0 direct
 * leg (NONE policy, no update) or the standard-indirect
 * MOD0--MOD3 leg (CMPT0/ARP0 policy, landed two-word AR deltas),
 * plus page-0 direct liveness (whose dma echo rides the LONG32
 * arm, not the DIRECT alias slot).
 * Does not own: other address kinds, transaction lifecycle,
 * endpoint I/O, architectural mutation, effect completion,
 * opcode dispatch, timing, observers, or diagnostics.
 * Resolution, candidate, and state arguments are borrowed for one
 * validation call; no pointer is retained and no state is
 * mutated.
 */

#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_LONG32_SINGLE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_LONG32_SINGLE_H

#include <stdbool.h>

struct ti_c54x_memory_address_resolution;
struct ti_c54x_op_memory_effect;
struct ti_c54x_memory_instruction_candidate;
struct ti_c54x_arch_state;

bool ti_c54x_memory_candidate_long32_single_shape_is_valid(
    const struct ti_c54x_memory_address_resolution *resolution,
    const struct ti_c54x_op_memory_effect *effect);

bool ti_c54x_memory_candidate_long32_single_direct_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_LONG32_SINGLE_H */
