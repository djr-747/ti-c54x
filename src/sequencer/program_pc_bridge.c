/*
 * Module contract
 *
 * Owns: checked conversion and transactional commit for the private
 * public-carrier/common-PC bridge. The current implementation admits only
 * one-word successors and delegates their +1 arithmetic and boundary policy
 * to sequential_pc.c.
 * Does not own: XPC or page interpretation, public CPU orchestration, opcode
 * decode or semantics, timing, buses, memory, architectural state, observers,
 * diagnostics, or allocation.
 */

#include "program_pc_bridge.h"

#include "sequential_pc.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool
ti_c54x_program_pc_bridge_effect_is_valid(
    const ti_c54x_program_pc_bridge_effect *effect)
{
    ti_c54x_sequential_pc_effect sequential;

    if (effect == NULL ||
        effect->program_word_count != UINT8_C(1) ||
        effect->public_pc_word_before > (uint32_t)UINT16_MAX ||
        effect->public_pc_word_after > (uint32_t)UINT16_MAX ||
        effect->public_pc_word_before !=
            (uint32_t)effect->common_pc_word_before ||
        effect->public_pc_word_after !=
            (uint32_t)effect->common_pc_word_after) {
        return false;
    }

    if (ti_c54x_sequencer_plan_sequential_pc(
            effect->common_pc_word_before, &sequential) !=
        TI_C54X_SEQUENTIAL_PC_OK) {
        return false;
    }
    return effect->common_pc_word_after == sequential.pc_word_after;
}

ti_c54x_program_pc_bridge_status
ti_c54x_program_pc_bridge_plan(
    uint32_t public_pc_word_before,
    uint8_t program_word_count,
    ti_c54x_program_pc_bridge_effect *effect)
{
    ti_c54x_sequential_pc_effect sequential;
    ti_c54x_program_pc_bridge_effect planned;
    ti_c54x_sequential_pc_status sequential_status;

    if (effect == NULL) {
        return TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT;
    }
    if (program_word_count != UINT8_C(1)) {
        return TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_PROGRAM_WORD_COUNT;
    }
    if (public_pc_word_before > (uint32_t)UINT16_MAX) {
        return TI_C54X_PROGRAM_PC_BRIDGE_UNSUPPORTED_CARRIER;
    }

    sequential_status = ti_c54x_sequencer_plan_sequential_pc(
        (uint16_t)public_pc_word_before, &sequential);
    if (sequential_status == TI_C54X_SEQUENTIAL_PC_UNRESOLVED_BOUNDARY) {
        return TI_C54X_PROGRAM_PC_BRIDGE_UNRESOLVED_BOUNDARY;
    }
    if (sequential_status != TI_C54X_SEQUENTIAL_PC_OK) {
        return TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT;
    }

    (void)memset(&planned, 0, sizeof(planned));
    planned.public_pc_word_before = public_pc_word_before;
    planned.common_pc_word_before = sequential.pc_word_before;
    planned.common_pc_word_after = sequential.pc_word_after;
    planned.public_pc_word_after = (uint32_t)sequential.pc_word_after;
    planned.program_word_count = program_word_count;
    *effect = planned;
    return TI_C54X_PROGRAM_PC_BRIDGE_OK;
}

ti_c54x_program_pc_bridge_status
ti_c54x_program_pc_bridge_apply(
    uint32_t *public_pc_word,
    const ti_c54x_program_pc_bridge_effect *effect)
{
    ti_c54x_sequential_pc_effect sequential;
    uint16_t common_pc_word;

    if (public_pc_word == NULL || effect == NULL) {
        return TI_C54X_PROGRAM_PC_BRIDGE_INVALID_ARGUMENT;
    }
    if (!ti_c54x_program_pc_bridge_effect_is_valid(effect)) {
        return TI_C54X_PROGRAM_PC_BRIDGE_INVALID_EFFECT;
    }
    if (*public_pc_word != effect->public_pc_word_before) {
        return TI_C54X_PROGRAM_PC_BRIDGE_STALE_SOURCE;
    }

    sequential.pc_word_before = effect->common_pc_word_before;
    sequential.pc_word_after = effect->common_pc_word_after;
    sequential.program_word_count = effect->program_word_count;
    common_pc_word = effect->common_pc_word_before;
    if (ti_c54x_sequencer_apply_sequential_pc(
            &common_pc_word, &sequential) != TI_C54X_SEQUENTIAL_PC_OK) {
        return TI_C54X_PROGRAM_PC_BRIDGE_INVALID_EFFECT;
    }

    *public_pc_word = effect->public_pc_word_after;
    return TI_C54X_PROGRAM_PC_BRIDGE_OK;
}
