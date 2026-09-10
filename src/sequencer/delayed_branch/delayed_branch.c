/*
 * Module contract
 *
 * Owns: the pure delayed-branch countdown transitions: init to the
 * disarmed idle state, arm with a two-word countdown and deferred
 * target (replacing any pending branch), saturating word-count
 * consume with take-effect reporting, and disarm.
 * Does not own: CPU/register storage, instruction decoding,
 * program-counter commit, memory or bus access, opcode identity,
 * timing, observers, or diagnostics.
 */

#include "delayed_branch.h"

#include <stddef.h>

bool ti_c54x_sequencer_delayed_branch_is_armed(
    const ti_c54x_sequencer_delayed_branch_state *state)
{
    return state != NULL && state->armed;
}

ti_c54x_sequencer_delayed_branch_status ti_c54x_sequencer_delayed_branch_init(
    ti_c54x_sequencer_delayed_branch_state *state)
{
    if (state == NULL) {
        return TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT;
    }
    state->armed = false;
    state->words_remaining = UINT8_C(0);
    state->deferred_pc_word = UINT16_C(0);
    return TI_C54X_SEQUENCER_DELAYED_BRANCH_OK;
}

ti_c54x_sequencer_delayed_branch_status ti_c54x_sequencer_delayed_branch_arm(
    ti_c54x_sequencer_delayed_branch_state *state,
    uint16_t deferred_pc_word)
{
    if (state == NULL) {
        return TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT;
    }
    state->armed = true;
    state->words_remaining =
        TI_C54X_SEQUENCER_DELAYED_BRANCH_SLOT_WORDS;
    state->deferred_pc_word = deferred_pc_word;
    return TI_C54X_SEQUENCER_DELAYED_BRANCH_OK;
}

ti_c54x_sequencer_delayed_branch_status
ti_c54x_sequencer_delayed_branch_disarm(
    ti_c54x_sequencer_delayed_branch_state *state)
{
    if (state == NULL) {
        return TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT;
    }
    state->armed = false;
    state->words_remaining = UINT8_C(0);
    state->deferred_pc_word = UINT16_C(0);
    return TI_C54X_SEQUENCER_DELAYED_BRANCH_OK;
}

ti_c54x_sequencer_delayed_branch_status
ti_c54x_sequencer_delayed_branch_consume(
    ti_c54x_sequencer_delayed_branch_state *state, uint8_t word_count,
    bool *took_effect)
{
    if (state == NULL || took_effect == NULL) {
        return TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT;
    }
    if (!state->armed || word_count == UINT8_C(0)) {
        return TI_C54X_SEQUENCER_DELAYED_BRANCH_INVALID_ARGUMENT;
    }
    if (word_count >= state->words_remaining) {
        state->armed = false;
        state->words_remaining = UINT8_C(0);
        *took_effect = true;
        return TI_C54X_SEQUENCER_DELAYED_BRANCH_OK;
    }
    state->words_remaining =
        (uint8_t)(state->words_remaining - word_count);
    *took_effect = false;
    return TI_C54X_SEQUENCER_DELAYED_BRANCH_OK;
}
