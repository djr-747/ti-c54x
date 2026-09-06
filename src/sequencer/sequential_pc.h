#ifndef TI_C54X_SEQUENCER_SEQUENTIAL_PC_H
#define TI_C54X_SEQUENCER_SEQUENTIAL_PC_H

/*
 * Module contract
 *
 * Owns: the private, checked 16-bit sequential program-counter retirement
 * seam for one-word instruction flow. A successful plan freezes the source PC,
 * its checked +1 destination, and the unit-bearing one-word width in a
 * value-owned staged effect; application commits that effect only when the
 * live PC still matches the frozen source.
 * Does not own: opcode identity or decode, public CPU carrier/XPC policy,
 * instruction timing, buses, memory, status registers, observers, or
 * diagnostics. The staged effect is private to this sequencer module and is
 * never part of the public CPU ABI.
 *
 * Planning source 0xffff returns UNRESOLVED_BOUNDARY and leaves the caller's
 * effect object unchanged. Every rejected application leaves the live PC
 * unchanged. No pointer is retained after either call returns.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_sequential_pc_status {
    TI_C54X_SEQUENTIAL_PC_OK = 0,
    TI_C54X_SEQUENTIAL_PC_INVALID_ARGUMENT = 1,
    TI_C54X_SEQUENTIAL_PC_INVALID_EFFECT = 2,
    TI_C54X_SEQUENTIAL_PC_STALE_SOURCE = 3,
    TI_C54X_SEQUENTIAL_PC_UNRESOLVED_BOUNDARY = 4
} ti_c54x_sequential_pc_status;

typedef struct ti_c54x_sequential_pc_effect {
    uint16_t pc_word_before;
    uint16_t pc_word_after;
    uint8_t program_word_count;
} ti_c54x_sequential_pc_effect;

/* Plan a checked one-word sequential PC transition without mutating input. */
ti_c54x_sequential_pc_status
ti_c54x_sequencer_plan_sequential_pc(
    uint16_t pc_word_before,
    ti_c54x_sequential_pc_effect *effect);

/* Validate and commit a previously staged transition transactionally. */
ti_c54x_sequential_pc_status
ti_c54x_sequencer_apply_sequential_pc(
    uint16_t *pc_word,
    const ti_c54x_sequential_pc_effect *effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_SEQUENTIAL_PC_H */
