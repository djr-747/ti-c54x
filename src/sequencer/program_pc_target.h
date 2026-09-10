#ifndef TI_C54X_SEQUENCER_PROGRAM_PC_TARGET_H
#define TI_C54X_SEQUENCER_PROGRAM_PC_TARGET_H

/*
 * Module contract
 *
 * Owns: the private bridge between the exact public 32-bit program-word
 * carrier and a copied 16-bit absolute branch target. It stages a
 * value-owned absolute successor and applies it transactionally only
 * when the caller-owned live carrier still matches the frozen source.
 * Does not own: XPC or page interpretation, truncation policy outside the
 * explicit carrier check, opcode identity or semantics, CPU orchestration,
 * delay slots, instruction timing, buses, memory, architectural state,
 * observers, diagnostics, or allocation. Public and common words in an
 * effect are copied values; no pointer is retained after either call
 * returns. Rejected planning leaves the caller's effect byte-for-byte
 * unchanged, and rejected application leaves the caller's live carrier
 * unchanged.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_program_pc_target_status {
    TI_C54X_PROGRAM_PC_TARGET_OK = 0,
    TI_C54X_PROGRAM_PC_TARGET_INVALID_ARGUMENT = 1,
    TI_C54X_PROGRAM_PC_TARGET_UNSUPPORTED_CARRIER = 2,
    TI_C54X_PROGRAM_PC_TARGET_INVALID_EFFECT = 3,
    TI_C54X_PROGRAM_PC_TARGET_STALE_SOURCE = 4
} ti_c54x_program_pc_target_status;

typedef struct ti_c54x_program_pc_target_effect {
    uint32_t public_pc_word_before;
    uint16_t common_pc_word_before;
    uint16_t common_pc_word_after;
    uint32_t public_pc_word_after;
} ti_c54x_program_pc_target_effect;

/* Stage a checked absolute successor without mutating the output on error. */
ti_c54x_program_pc_target_status ti_c54x_program_pc_target_plan(
    uint32_t public_pc_word_before,
    uint16_t target_pmad,
    ti_c54x_program_pc_target_effect *effect);

/* Validate and commit a staged successor transactionally to a live carrier. */
ti_c54x_program_pc_target_status ti_c54x_program_pc_target_apply(
    uint32_t *public_pc_word,
    const ti_c54x_program_pc_target_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_PROGRAM_PC_TARGET_H */
