/*
 * Module contract
 *
 * Owns: checked staging and transactional commit for the private
 * public-carrier/absolute-target bridge. A full 16-bit pmad is always
 * encodable, so planning fails only on caller-argument defects; the
 * live source check at apply preserves commit atomicity against a
 * carrier that moved between plan and apply.
 * Does not own: XPC or page interpretation, public CPU orchestration,
 * opcode decode or semantics, delay slots, timing, buses, memory,
 * architectural state, observers, diagnostics, or allocation.
 */

#include "program_pc_target.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool
ti_c54x_program_pc_target_effect_is_valid(
    const ti_c54x_program_pc_target_effect *effect)
{
    if (effect == NULL ||
        effect->public_pc_word_before > (uint32_t)UINT16_MAX ||
        effect->public_pc_word_after > (uint32_t)UINT16_MAX ||
        effect->public_pc_word_before !=
            (uint32_t)effect->common_pc_word_before ||
        effect->public_pc_word_after !=
            (uint32_t)effect->common_pc_word_after) {
        return false;
    }
    return true;
}

ti_c54x_program_pc_target_status ti_c54x_program_pc_target_plan(
    uint32_t public_pc_word_before,
    uint16_t target_pmad,
    ti_c54x_program_pc_target_effect *effect)
{
    ti_c54x_program_pc_target_effect planned;

    if (effect == NULL) {
        return TI_C54X_PROGRAM_PC_TARGET_INVALID_ARGUMENT;
    }
    if (public_pc_word_before > (uint32_t)UINT16_MAX) {
        return TI_C54X_PROGRAM_PC_TARGET_UNSUPPORTED_CARRIER;
    }

    (void)memset(&planned, 0, sizeof(planned));
    planned.public_pc_word_before = public_pc_word_before;
    planned.common_pc_word_before = (uint16_t)public_pc_word_before;
    planned.common_pc_word_after = target_pmad;
    planned.public_pc_word_after = (uint32_t)target_pmad;
    *effect = planned;
    return TI_C54X_PROGRAM_PC_TARGET_OK;
}

ti_c54x_program_pc_target_status ti_c54x_program_pc_target_apply(
    uint32_t *public_pc_word,
    const ti_c54x_program_pc_target_effect *effect)
{
    if (public_pc_word == NULL || effect == NULL) {
        return TI_C54X_PROGRAM_PC_TARGET_INVALID_ARGUMENT;
    }
    if (!ti_c54x_program_pc_target_effect_is_valid(effect)) {
        return TI_C54X_PROGRAM_PC_TARGET_INVALID_EFFECT;
    }
    if (*public_pc_word != effect->public_pc_word_before) {
        return TI_C54X_PROGRAM_PC_TARGET_STALE_SOURCE;
    }

    *public_pc_word = effect->public_pc_word_after;
    return TI_C54X_PROGRAM_PC_TARGET_OK;
}
