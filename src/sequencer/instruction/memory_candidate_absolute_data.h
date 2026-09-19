/* Module contract
 *
 * Owns: structural and live-state validation of one copied
 * absolute-DATA WORD16 resolution against its memory effect: the
 * second-word address with the ABSOLUTE source echo (full
 * address, no page/offset split), the NONE policy (no CPL/DP
 * observation),
 * the read-phase MMR sidecar (STORE or LOAD) with a routable
 * MMR number, and the sequential PC.
 * Does not own: other address kinds, transaction lifecycle,
 * endpoint I/O, architectural mutation, effect completion,
 * opcode dispatch, timing, observers, or diagnostics.
 * Resolution, candidate, and state arguments are borrowed for one
 * validation call; no pointer is retained and no state is
 * mutated.
 */

#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_ABSOLUTE_DATA_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_ABSOLUTE_DATA_H

#include <stdbool.h>

struct ti_c54x_memory_address_resolution;
struct ti_c54x_op_memory_effect;
struct ti_c54x_memory_instruction_candidate;
struct ti_c54x_arch_state;

bool ti_c54x_memory_candidate_absolute_data_shape_is_valid(
    const struct ti_c54x_memory_address_resolution *resolution,
    const struct ti_c54x_op_memory_effect *effect);

bool ti_c54x_memory_candidate_absolute_data_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_ABSOLUTE_DATA_H */
