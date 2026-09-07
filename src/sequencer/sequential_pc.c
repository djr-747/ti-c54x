/*
 * Module contract
 *
 * Owns: checked planning and transactional commit of the private 16-bit
 * sequential program-counter width-sized effect.
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
    uint32_t expected_after;

    if (effect == NULL || effect->program_word_count == UINT8_C(0)) {
        return false;
    }
    expected_after = (uint32_t)effect->pc_word_before +
                     (uint32_t)effect->program_word_count;
    if (expected_after > (uint32_t)UINT16_MAX) {
        return false;
    }
    return effect->pc_word_after == (uint16_t)expected_after;
}

static ti_c54x_sequential_pc_status
ti_c54x_sequencer_plan_sequential_pc_after(
    uint16_t pc_word_before,
    uint8_t program_word_count,
    uint16_t *pc_word_after)
{
    uint32_t after;

    if (pc_word_after == NULL) {
        return TI_C54X_SEQUENTIAL_PC_INVALID_ARGUMENT;
    }
    if (program_word_count == UINT8_C(0)) {
        return TI_C54X_SEQUENTIAL_PC_UNSUPPORTED_PROGRAM_WORD_COUNT;
    }
    after = (uint32_t)pc_word_before + (uint32_t)program_word_count;
    if (after > (uint32_t)UINT16_MAX) {
        return TI_C54X_SEQUENTIAL_PC_UNRESOLVED_BOUNDARY;
    }

    *pc_word_after = (uint16_t)after;
    return TI_C54X_SEQUENTIAL_PC_OK;
}

ti_c54x_sequential_pc_status
ti_c54x_sequencer_plan_sequential_pc(
    uint16_t pc_word_before,
    uint8_t program_word_count,
    ti_c54x_sequential_pc_effect *effect)
{
    ti_c54x_sequential_pc_effect planned;
    ti_c54x_sequential_pc_status status;

    (void)memset(&planned, 0, sizeof(planned));

    if (effect == NULL) {
        return TI_C54X_SEQUENTIAL_PC_INVALID_ARGUMENT;
    }
    status = ti_c54x_sequencer_plan_sequential_pc_after(
        pc_word_before, program_word_count, &planned.pc_word_after);
    if (status != TI_C54X_SEQUENTIAL_PC_OK) {
        return status;
    }

    planned.pc_word_before = pc_word_before;
    planned.program_word_count = program_word_count;
    *effect = planned;
    return TI_C54X_SEQUENTIAL_PC_OK;
}

ti_c54x_sequential_pc_status
ti_c54x_sequencer_plan_sequential_pc_trusted(
    uint16_t pc_word_before,
    uint8_t program_word_count,
    uint16_t *pc_word_after)
{
    return ti_c54x_sequencer_plan_sequential_pc_after(
        pc_word_before, program_word_count, pc_word_after);
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
