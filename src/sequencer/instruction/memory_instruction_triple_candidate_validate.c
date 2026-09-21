/*
 * Module contract
 *
 * Owns: structural and live-state validation of the copied
 * dual-data-plus-program triple arm. Structural validation
 * proves both canonical DATA READ requests, the absolute
 * PROGRAM READ request, both MOD0/MOD1/MOD2/MOD3 address
 * snapshots, and both value-owned commit descriptors against
 * the copied mode/selector/address shadows (MOD3 additionally
 * rechecks the AR0/BK shadows); live validation rechecks only
 * the planned PC and the two source AR snapshots before a
 * resume. The X/Y commit rules mirror the dual route
 * verbatim; only the address-kind gate and field sourcing
 * differ.
 * Does not own: candidate construction, transaction lifecycle, address
 * planning, CMPT/ARP policy, endpoint I/O, architectural mutation, opcode
 * semantics, timing, observers, diagnostics, or allocation. All arguments
 * are borrowed for one call and no pointer is retained.
 */

#include "memory_instruction_triple_candidate_validate.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "isa/ops/memory/memory_effect.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/program_pc_bridge.h"

#include "address/indirect_dual_mod3.h"

static bool
triple_request_side_is_valid(const ti_c54x_bus_word16_request *request,
                             bool program)
{
    if (request == NULL || !ti_c54x_bus_word16_request_is_valid(request) ||
        request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        request->write_value_word != UINT16_C(0)) {
        return false;
    }
    if (program) {
        return request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM;
    }
    return request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA;
}

static bool
triple_mode_is_valid(uint8_t mode)
{
    return mode <= UINT8_C(3);
}

static bool
triple_no_update_commit_is_valid(
    const ti_c54x_sequencer_address_commit *commit)
{
    return commit != NULL &&
           commit->target == TI_C54X_ADDRESS_UPDATE_NONE &&
           commit->ar_index == UINT8_C(0) &&
           commit->value_after == UINT16_C(0);
}

static bool
triple_side_commit_matches(uint8_t mode, uint8_t ar, uint32_t address_word,
                           bool aliased_y, uint16_t ar0_shadow,
                           uint16_t bk_shadow,
                           const ti_c54x_sequencer_address_commit *commit)
{
    if (commit == NULL || address_word > UINT32_C(0xffff)) {
        return false;
    }
    /* A same-selector alias carries one X-owned update; the Y side reads the
     * same frozen address but stages no independent commit. */
    if (aliased_y || mode == UINT8_C(0)) {
        return triple_no_update_commit_is_valid(commit);
    }
    if (mode == UINT8_C(3)) {
        uint16_t expected_after = UINT16_C(0);

        /* The planner rejects MOD3 boundary violations, so a forged
         * boundary-crossing commit cannot validate here either. */
        if (ti_c54x_indirect_dual_mod3_after((uint16_t)address_word,
                                            ar0_shadow, bk_shadow,
                                            &expected_after) !=
            TI_C54X_ADDRESS_STATUS_OK) {
            return false;
        }
        /* The documented BK=0/AR0=0 case performs no modification. */
        if (ar0_shadow == UINT16_C(0) && bk_shadow == UINT16_C(0)) {
            return triple_no_update_commit_is_valid(commit);
        }
        return commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
               commit->ar_index == ar && commit->value_after == expected_after;
    }
    if (mode == UINT8_C(1)) {
        /* The resolver rejects the zero boundary, so a forged zero address
         * cannot wrap into a pending candidate here. */
        if (address_word == UINT32_C(0)) {
            return false;
        }
        return commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
               commit->ar_index == ar &&
               commit->value_after ==
                   (uint16_t)(address_word - UINT32_C(1));
    }
    if (mode == UINT8_C(2)) {
        /* The resolver rejects the 0xffff boundary; require the exact
         * non-wrapping successor. */
        if (address_word == UINT32_C(0xffff)) {
            return false;
        }
        return commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
               commit->ar_index == ar &&
               commit->value_after ==
                   (uint16_t)(address_word + UINT32_C(1));
    }
    return false;
}

static bool triple_arm_is_valid(
    const struct ti_c54x_op_memory_address_spec *spec)
{
    return spec != NULL &&
           spec->kind ==
               TI_C54X_OP_MEMORY_ADDRESS_DUAL_PROGRAM_TRIPLE_WORD16 &&
           triple_mode_is_valid(spec->dual_program.x_mode) &&
           triple_mode_is_valid(spec->dual_program.y_mode) &&
           spec->dual_program.x_ar >= UINT8_C(2) &&
           spec->dual_program.x_ar <= UINT8_C(5) &&
           spec->dual_program.y_ar >= UINT8_C(2) &&
           spec->dual_program.y_ar <= UINT8_C(5) &&
           spec->dual_program.reserved_half == UINT16_C(0) &&
           spec->dual_program.reserved_word1 == UINT32_C(0) &&
           spec->dual_program.reserved_word2 == UINT32_C(0) &&
           spec->dual_program.reserved_word3 == UINT32_C(0);
}

bool
ti_c54x_memory_instruction_triple_snapshot_commits_match_spec(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_address_spec *spec)
{
    if (snapshot == NULL || !triple_arm_is_valid(spec)) {
        return false;
    }
    if (!triple_side_commit_matches(
            spec->dual_program.x_mode, spec->dual_program.x_ar,
            snapshot->x_request.address_word, false, snapshot->ar0_shadow,
            snapshot->bk_shadow, &snapshot->x_commit)) {
        return false;
    }
    return triple_side_commit_matches(
        spec->dual_program.y_mode, spec->dual_program.y_ar,
        snapshot->y_request.address_word,
        spec->dual_program.x_ar == spec->dual_program.y_ar,
        snapshot->ar0_shadow, snapshot->bk_shadow, &snapshot->y_commit);
}

bool
ti_c54x_memory_instruction_triple_candidate_shape_is_valid(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_effect *effect)
{
    const ti_c54x_op_memory_address_spec *spec;

    if (snapshot == NULL || effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_DUAL_PROGRAM_TRIPLE_WORD16 ||
        effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        return false;
    }
    spec = &effect->address_spec;
    if (!triple_arm_is_valid(spec)) {
        return false;
    }
    if (!triple_request_side_is_valid(&snapshot->x_request, false) ||
        !triple_request_side_is_valid(&snapshot->y_request, false)) {
        return false;
    }
    /* A selector alias is read twice during planning.  It is valid only if
     * both frozen snapshots agree, proving the pair was not torn. */
    if (spec->dual_program.x_ar == spec->dual_program.y_ar &&
        snapshot->x_request.address_word !=
            snapshot->y_request.address_word) {
        return false;
    }
    return ti_c54x_memory_instruction_triple_snapshot_commits_match_spec(
        snapshot, spec);
}

static bool
pc_candidate_matches_live(
    const ti_c54x_program_pc_bridge_effect *pc,
    const ti_c54x_arch_state *state)
{
    ti_c54x_program_pc_bridge_effect copy;
    uint32_t live_pc;

    if (pc == NULL || state == NULL ||
        pc->public_pc_word_before != state->pc_word) {
        return false;
    }
    copy = *pc;
    live_pc = state->pc_word;
    return ti_c54x_program_pc_bridge_apply(&live_pc, &copy) ==
               TI_C54X_PROGRAM_PC_BRIDGE_OK &&
           live_pc == pc->public_pc_word_after;
}

bool
ti_c54x_memory_instruction_triple_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state)
{
    const ti_c54x_dual_memory_address_snapshot *snapshot;
    const ti_c54x_op_memory_address_spec *spec;
    uint16_t selected_ar;

    if (candidate == NULL || state == NULL ||
        !ti_c54x_memory_instruction_triple_candidate_shape_is_valid(
            &candidate->dual_address_snapshot,
            &candidate->effect.payload.memory) ||
        !pc_candidate_matches_live(&candidate->pc_effect, state)) {
        return false;
    }
    snapshot = &candidate->dual_address_snapshot;
    spec = &candidate->effect.payload.memory.address_spec;
    if (ti_c54x_arch_state_read_ar(
            state, spec->dual_program.x_ar,
            &selected_ar) != TI_C54X_ARCH_STATE_OK ||
        selected_ar !=
            snapshot->x_request.address_word) {
        return false;
    }
    if (ti_c54x_arch_state_read_ar(
            state, spec->dual_program.y_ar,
            &selected_ar) != TI_C54X_ARCH_STATE_OK ||
        selected_ar !=
            snapshot->y_request.address_word) {
        return false;
    }
    return true;
}
