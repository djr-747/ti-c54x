#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_STACK_PAIR_CANDIDATE_VALIDATE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_STACK_PAIR_CANDIDATE_VALIDATE_H

/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied stack-pair
 * candidates plus the pair snapshot carrier. Both legs live in the
 * snapshot: the TOS stack-leg resolution (whose SP commit the
 * architecture commit applies) and the Smem DATA-leg resolution,
 * with the second-leg-issued flag. The candidate's generic
 * address-resolution slot shares union storage with the snapshot
 * and holds no stack-pair commit. Shape re-derives both leg specs
 * from the frozen arm and input; liveness pins the frozen SP
 * snapshot, the sequential PC, and the DATA leg against live
 * state on every resume. All inputs are borrowed for one call.
 * Does not own: transaction lifecycle, endpoint traffic,
 * architectural mutation, effect completion, opcode dispatch,
 * timing, observers, diagnostics, allocation, or global state.
 */

#include <stdbool.h>
#include <stdint.h>

#include "sequencer/memory/address_resolution.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ti_c54x_memory_instruction_candidate;
struct ti_c54x_arch_state;

/* Pair snapshot retained by a prepared root transaction: the
 * resolved TOS stack-leg resolution, the resolved Smem DATA-leg
 * resolution, plus the second-leg flag. Both legs live here
 * because the generic slot aliases this storage. */
typedef struct ti_c54x_stack_pair_address_snapshot {
    ti_c54x_memory_address_resolution stack_resolution;
    ti_c54x_memory_address_resolution data_resolution;
    uint8_t second_leg_issued;
} ti_c54x_stack_pair_address_snapshot;

bool ti_c54x_memory_instruction_stack_pair_candidate_shape_is_valid(
    const struct ti_c54x_memory_instruction_candidate *candidate);

bool ti_c54x_memory_instruction_stack_pair_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state);

bool ti_c54x_memory_instruction_stack_pair_data_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state,
    const ti_c54x_memory_address_resolution *resolution);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_STACK_PAIR_CANDIDATE_VALIDATE_H */
