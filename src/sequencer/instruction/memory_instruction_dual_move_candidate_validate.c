/*
 * Module contract
 *
 * Owns: structural and live-state validation of the copied
 * dual-move candidate arm; see
 * memory_instruction_dual_move_candidate_validate.h.
 * Does not own: candidate construction, transaction lifecycle,
 * address planning, CMPT/ARP policy, endpoint I/O,
 * architectural mutation, opcode semantics, timing, observers,
 * diagnostics, or allocation. All arguments are borrowed for
 * one call and no pointer is retained.
 */

#include "memory_instruction_dual_move_candidate_validate.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "isa/ops/memory/memory_effect.h"
#include "sequencer/instruction/memory_instruction_dual_candidate_validate.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/program_pc_bridge.h"

static bool
move_read_side_is_valid(const ti_c54x_bus_word16_request *request)
{
    if (request == NULL || !ti_c54x_bus_word16_request_is_valid(request) ||
        request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        request->write_value_word != UINT16_C(0)) {
        return false;
    }
    return true;
}

static bool
move_write_side_is_valid(const ti_c54x_bus_word16_request *request)
{
    if (request == NULL || !ti_c54x_bus_word16_request_is_valid(request) ||
        request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return false;
    }
    return true;
}

bool
ti_c54x_memory_instruction_dual_move_candidate_shape_is_valid(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_effect *effect)
{
    const ti_c54x_op_memory_address_spec *spec;

    if (snapshot == NULL || effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_MOVE_PAIR_WORD16 ||
        effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ||
        effect->complete != NULL || effect->complete_dual != NULL) {
        return false;
    }
    spec = &effect->address_spec;
    if (spec->dual.x_mode > UINT8_C(3) || spec->dual.y_mode > UINT8_C(3) ||
        spec->dual.x_ar < UINT8_C(2) || spec->dual.x_ar > UINT8_C(5) ||
        spec->dual.y_ar < UINT8_C(2) || spec->dual.y_ar > UINT8_C(5) ||
        spec->dual.reserved_word0 != UINT32_C(0) ||
        spec->dual.reserved_word1 != UINT32_C(0) ||
        spec->dual.reserved_word2 != UINT32_C(0) ||
        spec->dual.reserved_word3 != UINT32_C(0)) {
        return false;
    }
    if (!move_read_side_is_valid(&snapshot->x_request) ||
        !move_write_side_is_valid(&snapshot->y_request)) {
        return false;
    }
    /* A selector alias is read twice during planning. It is valid only if
     * both frozen snapshots agree, proving the pair was not torn. */
    if (spec->dual.x_ar == spec->dual.y_ar &&
        snapshot->x_request.address_word !=
            snapshot->y_request.address_word) {
        return false;
    }
    return ti_c54x_memory_instruction_dual_snapshot_commits_match_spec(
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
ti_c54x_memory_instruction_dual_move_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state)
{
    const ti_c54x_dual_memory_address_snapshot *snapshot;
    const ti_c54x_op_memory_address_spec *spec;
    uint16_t selected_ar;

    if (candidate == NULL || state == NULL ||
        !ti_c54x_memory_instruction_dual_move_candidate_shape_is_valid(
            &candidate->dual_address_snapshot,
            &candidate->effect.payload.memory) ||
        !pc_candidate_matches_live(&candidate->pc_effect, state)) {
        return false;
    }
    snapshot = &candidate->dual_address_snapshot;
    spec = &candidate->effect.payload.memory.address_spec;
    if (ti_c54x_arch_state_read_ar(
            state, spec->dual.x_ar,
            &selected_ar) != TI_C54X_ARCH_STATE_OK ||
        selected_ar !=
            snapshot->x_request.address_word) {
        return false;
    }
    if (ti_c54x_arch_state_read_ar(
            state, spec->dual.y_ar,
            &selected_ar) != TI_C54X_ARCH_STATE_OK ||
        selected_ar !=
            snapshot->y_request.address_word) {
        return false;
    }
    return true;
}
