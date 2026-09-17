#ifndef TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_SINGLE_LEG_H
#define TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_SINGLE_LEG_H

/*
 * Module contract
 *
 * Owns: structural validation of one copied single-leg DATA address
 * resolution against its memory effect: single-indirect MOD0--MOD3,
 * AR0-indexed MOD5/MOD6, circular MOD8/MOD10, reverse-carry
 * MOD4/MOD7, LONG32, X-only dual MOD0--MOD3, and mode-independent
 * STACK_PUSH/STACK_POP operands with their delayed-branch twins.
 * Page-0 and DP-relative direct arms delegate to the
 * memory_candidate_direct module.
 * Does not own: transaction lifecycle, endpoint I/O, architectural
 * mutation, effect completion, opcode dispatch, timing, observers,
 * or diagnostics. Resolution and effect arguments are borrowed for
 * one validation call; no pointer is retained and no state is
 * mutated.
 */

#include <stdbool.h>

struct ti_c54x_memory_address_resolution;
struct ti_c54x_op_memory_effect;

bool ti_c54x_memory_candidate_single_leg_shape_is_valid(
    const struct ti_c54x_memory_address_resolution *resolution,
    const struct ti_c54x_op_memory_effect *effect);

#endif /* TI_C54X_SEQUENCER_INSTRUCTION_MEMORY_CANDIDATE_SINGLE_LEG_H */
