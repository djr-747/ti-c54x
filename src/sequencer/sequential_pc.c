/*
 * Module contract
 *
 * Owns: checked planning and transactional commit of the private 16-bit
 * sequential program-counter +1 effect.
 * Does not own: opcode decode or semantics, public CPU state, timing, buses,
 * memory, observers, or diagnostics.
 */

#include "sequential_pc.h"

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

static bool
ti_c54x_sequential_pc_effect_is_valid(
    const ti_c54x_sequential_pc_effect *effect)
{
    if (effect == NULL || effect->program_word_count != UINT8_C(1) ||
        effect->pc_word_before == UINT16_MAX) {
        return false;
    }
    return effect->pc_word_after ==
           (uint16_t)(effect->pc_word_before + UINT16_C(1));
}

ti_c54x_sequential_pc_status
ti_c54x_sequencer_plan_sequential_pc(
    uint16_t pc_word_before,
    ti_c54x_sequential_pc_effect *effect)
{
    ti_c54x_sequential_pc_effect planned;

    (void)memset(&planned, 0, sizeof(planned));

    if (effect == NULL) {
        return TI_C54X_SEQUENTIAL_PC_INVALID_ARGUMENT;
    }
    if (pc_word_before == UINT16_MAX) {
        return TI_C54X_SEQUENTIAL_PC_UNRESOLVED_BOUNDARY;
    }

    planned.pc_word_before = pc_word_before;
    planned.pc_word_after = (uint16_t)(pc_word_before + UINT16_C(1));
    planned.program_word_count = UINT8_C(1);
    *effect = planned;
    return TI_C54X_SEQUENTIAL_PC_OK;
}

ti_c54x_sequential_pc_status
ti_c54x_sequencer_apply_sequential_pc(
    uint16_t *pc_word,
    const ti_c54x_sequential_pc_effect *effect)
{
    if (pc_word == NULL || effect == NULL) {
        return TI_C54X_SEQUENTIAL_PC_INVALID_ARGUMENT;
    }
    if (!ti_c54x_sequential_pc_effect_is_valid(effect)) {
        return TI_C54X_SEQUENTIAL_PC_INVALID_EFFECT;
    }
    if (*pc_word != effect->pc_word_before) {
        return TI_C54X_SEQUENTIAL_PC_STALE_SOURCE;
    }

    *pc_word = effect->pc_word_after;
    return TI_C54X_SEQUENTIAL_PC_OK;
}
