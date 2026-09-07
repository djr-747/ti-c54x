#ifndef TI_C54X_SEQUENCER_PROGRAM_PC_BRIDGE_H
#define TI_C54X_SEQUENCER_PROGRAM_PC_BRIDGE_H

/*
 * Module contract
 *
 * Owns: the private bridge between the exact public 32-bit program-word
 * carrier and the common 16-bit sequential program counter. It stages a
 * value-owned width-sized successor (one through 255 program words) and
 * applies it transactionally only when the caller-owned live carrier still
 * matches the frozen source.
 * Does not own: XPC or page interpretation, truncation policy outside the
 * explicit carrier check, opcode identity or semantics, CPU orchestration,
 * instruction timing, buses, memory, architectural state, observers,
 * diagnostics, or allocation. Public and common words in an effect are
 * copied values; no pointer is retained after either call returns. Rejected
 * planning leaves the caller's effect byte-for-byte unchanged, and rejected
 * application leaves the caller's live carrier unchanged.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_program_pc_bridge_status {
    TI_C54X_PROGRAM_PC_BRIDGE_OK = 0,
    TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT = 1,
    TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_CARRIER = 2,
    TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_PROGRAM_WORD_COUNT = 3,
    TI_C54X_PROGRAM_PC_BRIDGE_UNRESOLVED_BOUNDARY = 4,
    TI_C54X_PROGRAM_PC_BRIDGE_INVALID_EFFECT = 5,
    TI_C54X_PROGRAM_PC_BRIDGE_STALE_SOURCE = 6
} ti_c54x_program_pc_bridge_status;

typedef struct ti_c54x_program_pc_bridge_effect {
    uint32_t public_pc_word_before;
    uint16_t common_pc_word_before;
    uint16_t common_pc_word_after;
    uint32_t public_pc_word_after;
    uint8_t program_word_count;
} ti_c54x_program_pc_bridge_effect;

/* Stage a checked sequential successor without mutating the output on error. */
ti_c54x_program_pc_bridge_status ti_c54x_program_pc_bridge_plan(
    uint32_t public_pc_word_before,
    uint8_t program_word_count,
    ti_c54x_program_pc_bridge_effect *effect);

/* Build the same checked plan for the trusted CPU path. Rejections preserve
 * output; the implementation and boundary arithmetic are shared with plan(). */
ti_c54x_program_pc_bridge_status ti_c54x_program_pc_bridge_plan_trusted(
    uint32_t public_pc_word_before,
    uint8_t program_word_count,
    ti_c54x_program_pc_bridge_effect *effect);

/* Validate and commit a staged successor transactionally to a live carrier. */
ti_c54x_program_pc_bridge_status ti_c54x_program_pc_bridge_apply(
    uint32_t *public_pc_word,
    const ti_c54x_program_pc_bridge_effect *effect);

/* Commit an effect returned by a successful plan without recomputing it.
 * Private caller precondition: planned_effect is the untouched value-owned
 * result of plan() or plan_trusted(), and the live carrier is its source.
 * The C type cannot encode that provenance, so this seam intentionally checks
 * only the live source and performs no effect validation or arithmetic. */
ti_c54x_program_pc_bridge_status ti_c54x_program_pc_bridge_apply_trusted(
    uint32_t *public_pc_word,
    const ti_c54x_program_pc_bridge_effect *planned_effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_PROGRAM_PC_BRIDGE_H */
