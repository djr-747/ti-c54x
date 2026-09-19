/* Module contract
 *
 * Owns: structural and live-state validation of one copied
 * atomic absolute WORD16 resolution against its memory
 * effect; see memory_candidate_atomic_absolute.h.
 * Does not own: other address kinds, transaction lifecycle,
 * endpoint I/O, architectural mutation, effect completion,
 * opcode dispatch, timing, observers, diagnostics, allocation,
 * or callbacks.
 */

#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_ATOMIC_ABSOLUTE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_ATOMIC_ABSOLUTE_H

#include <stdbool.h>

#include "sequencer/memory/address_resolution.h"

struct ti_c54x_op_memory_effect;
struct ti_c54x_memory_instruction_candidate;
struct ti_c54x_arch_state;

bool ti_c54x_memory_candidate_atomic_absolute_shape_is_valid(
    const ti_c54x_memory_address_resolution *resolution,
    const struct ti_c54x_op_memory_effect *effect);

bool ti_c54x_memory_candidate_atomic_absolute_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_ATOMIC_ABSOLUTE_H */
