#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_SMEM_ABSOLUTE_PAIR_CANDIDATE_VALIDATE_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_SMEM_ABSOLUTE_PAIR_CANDIDATE_VALIDATE_H

/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied
 * Smem-to-absolute pair candidates plus the pair snapshot
 * carrier. Both legs live in the snapshot: the Smem DATA-leg
 * resolution (direct-DP or single-indirect) and the absolute
 * dmad-leg resolution, with the second-leg-issued flag. The
 * candidate's generic address-resolution slot shares union
 * storage with the snapshot and holds no pair commit. Shape
 * re-derives both leg specs from the frozen arm and input;
 * liveness pins the frozen DP/AR snapshots, the sequential PC,
 * and the dmad echo against live state on every resume. All
 * inputs are borrowed for one call.
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
 * resolved Smem DATA-leg resolution, the resolved absolute
 * dmad-leg resolution, plus the second-leg flag. Both legs live
 * here because the generic slot aliases this storage. */
typedef struct ti_c54x_smem_absolute_pair_address_snapshot {
    ti_c54x_memory_address_resolution data_resolution;
    ti_c54x_memory_address_resolution dmad_resolution;
    uint8_t second_leg_issued;
} ti_c54x_smem_absolute_pair_address_snapshot;

bool ti_c54x_memory_instruction_smem_absolute_pair_candidate_shape_is_valid(
    const struct ti_c54x_memory_instruction_candidate *candidate);

bool ti_c54x_memory_instruction_smem_absolute_pair_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state);

bool ti_c54x_memory_instruction_smem_absolute_pair_data_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state,
    const ti_c54x_memory_address_resolution *resolution);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_INSTRUCTION_SMEM_ABSOLUTE_PAIR_CANDIDATE_VALIDATE_H */
