#ifndef TI_C54X_SEQUENCER_DELAYED_BRANCH_DELAYED_BRANCH_H
#define TI_C54X_SEQUENCER_DELAYED_BRANCH_DELAYED_BRANCH_H

/*
 * Module contract
 *
 * Owns: the pure value-owned delayed-branch countdown: a disarmed
 * idle state, arming with a 16-bit deferred program-counter target
 * and a two-program-word slot countdown, word-count consume with
 * take-effect reporting, and disarm. Arming replaces any pending
 * branch; a consume wider than the remaining words still takes
 * effect (saturating, for malformed slot fills). Disarmed and
 * zero-width consumes are rejected.
 * Does not own: CPU/register storage, instruction decoding,
 * program-counter commit, memory or bus access, opcode identity,
 * timing, observers, or diagnostics. Inputs are copied; all result
 * fields are value-owned by the caller-provided objects.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ti_c54x_sequencer_delayed_branch_status {
    TI_C54X_SEQUENCER_DELAYED_BRANCH_OK = 0,
    TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT = 1
} ti_c54x_sequencer_delayed_branch_status;

/* Every delayed branch (TI SPRU172C `D` suffix, SPRU131G §6.3.1)
 * executes the two program words following the branch before the
 * PC transfer takes effect. */
#define TI_C54X_SEQUENCER_DELAYED_BRANCH_SLOT_WORDS UINT8_C(2)

typedef struct ti_c54x_sequencer_delayed_branch_state {
    bool armed;
    uint8_t words_remaining;
    uint16_t deferred_pc_word;
} ti_c54x_sequencer_delayed_branch_state;

bool ti_c54x_sequencer_delayed_branch_is_armed(
    const ti_c54x_sequencer_delayed_branch_state *state);

ti_c54x_sequencer_delayed_branch_status ti_c54x_sequencer_delayed_branch_init(
    ti_c54x_sequencer_delayed_branch_state *state);

ti_c54x_sequencer_delayed_branch_status ti_c54x_sequencer_delayed_branch_arm(
    ti_c54x_sequencer_delayed_branch_state *state,
    uint16_t deferred_pc_word);

ti_c54x_sequencer_delayed_branch_status
ti_c54x_sequencer_delayed_branch_disarm(
    ti_c54x_sequencer_delayed_branch_state *state);

/* Consume retired delay-slot words. Reports through took_effect
 * whether the countdown reached zero (saturating) and the deferred
 * target now applies. A disarmed state or a zero word count is
 * rejected without mutation. */
ti_c54x_sequencer_delayed_branch_status
ti_c54x_sequencer_delayed_branch_consume(
    ti_c54x_sequencer_delayed_branch_state *state, uint8_t word_count,
    bool *took_effect);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_SEQUENCER_DELAYED_BRANCH_DELAYED_BRANCH_H */
