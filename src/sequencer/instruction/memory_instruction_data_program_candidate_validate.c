/*
 * Module contract
 *
 * Owns: structural and live-state validation of the copied
 * DATA+PROGRAM pair candidate arm. Structural validation proves the
 * canonical DATA READ request, the absolute PROGRAM READ request,
 * the single Smem address snapshot, and both value-owned commit
 * descriptors against the copied leg-selector/address shadows;
 * live validation rechecks only the planned PC and the Smem source
 * AR snapshot before a resume.
 * Does not own: candidate construction, transaction lifecycle,
 * address planning, CMPT/ARP policy, endpoint I/O, architectural
 * mutation, opcode semantics, timing, observers, diagnostics, or
 * allocation. All arguments are borrowed for one call and no
 * pointer is retained.
 */

#include "memory_instruction_data_program_candidate_validate.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "isa/ops/memory/memory_effect.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/program_pc_bridge.h"

static bool
data_request_side_is_valid(const ti_c54x_bus_word16_request *request)
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
program_request_side_is_valid(const ti_c54x_bus_word16_request *request)
{
    if (request == NULL || !ti_c54x_bus_word16_request_is_valid(request) ||
        request->space != TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM ||
        request->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        request->write_value_word != UINT16_C(0)) {
        return false;
    }
    return true;
}

static bool
pair_spec_is_valid(const struct ti_c54x_op_memory_address_spec *spec)
{
    if (spec == NULL ||
        spec->kind !=
            TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16 ||
        spec->pair.indirect > UINT8_C(1) ||
        spec->pair.smem_arf > UINT8_C(7) ||
        spec->pair.smem_dma7 > UINT8_C(0x7f) ||
        spec->pair.reserved_half != UINT16_C(0) ||
        spec->pair.reserved_word0 != UINT32_C(0) ||
        spec->pair.reserved_word1 != UINT32_C(0) ||
        spec->pair.reserved_word2 != UINT32_C(0)) {
        return false;
    }
    if (spec->pair.indirect == UINT8_C(0)) {
        return spec->pair.smem_mode == UINT8_C(0) &&
               spec->pair.smem_arf == UINT8_C(0);
    }
    return spec->pair.smem_mode <= UINT8_C(2) &&
           spec->pair.smem_dma7 == UINT8_C(0);
}

static bool
no_update_commit_is_valid(const ti_c54x_sequencer_address_commit *commit)
{
    return commit != NULL &&
           commit->target == TI_C54X_ADDRESS_UPDATE_NONE &&
           commit->ar_index == UINT8_C(0) &&
           commit->value_after == UINT16_C(0);
}

static bool
smem_commit_matches(uint8_t indirect, uint8_t mode, uint8_t arf,
                    uint32_t address_word,
                    const ti_c54x_sequencer_address_commit *commit)
{
    if (commit == NULL || address_word > UINT32_C(0xffff)) {
        return false;
    }
    /* The direct leg and MOD0 perform no update. */
    if (indirect == UINT8_C(0) || mode == UINT8_C(0)) {
        return no_update_commit_is_valid(commit);
    }
    if (mode == UINT8_C(1)) {
        /* The resolver rejects the zero boundary, so a forged zero
         * address cannot wrap into a pending candidate here. */
        if (address_word == UINT32_C(0)) {
            return false;
        }
        return commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
               commit->ar_index == arf &&
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
               commit->ar_index == arf &&
               commit->value_after ==
                   (uint16_t)(address_word + UINT32_C(1));
    }
    return false;
}

bool
ti_c54x_memory_instruction_data_program_snapshot_commits_match_spec(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_address_spec *spec)
{
    if (snapshot == NULL || !pair_spec_is_valid(spec)) {
        return false;
    }
    if (!smem_commit_matches(spec->pair.indirect, spec->pair.smem_mode,
                             spec->pair.smem_arf,
                             snapshot->x_request.address_word,
                             &snapshot->x_commit)) {
        return false;
    }
    /* The PROGRAM leg never carries a post-access update. */
    return no_update_commit_is_valid(&snapshot->y_commit);
}

bool
ti_c54x_memory_instruction_data_program_candidate_shape_is_valid(
    const ti_c54x_dual_memory_address_snapshot *snapshot,
    const struct ti_c54x_op_memory_effect *effect)
{
    const ti_c54x_op_memory_address_spec *spec;

    if (snapshot == NULL || effect == NULL ||
        effect->address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_DATA_PROGRAM_PAIR_WORD16 ||
        effect->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        return false;
    }
    spec = &effect->address_spec;
    if (!pair_spec_is_valid(spec)) {
        return false;
    }
    if (!data_request_side_is_valid(&snapshot->x_request) ||
        !program_request_side_is_valid(&snapshot->y_request)) {
        return false;
    }
    /* Pin both frozen addresses to the copied leg shadows: the DATA
     * address to the Smem leg descriptor and the PROGRAM address to
     * the verbatim pmad. */
    if (spec->pair.indirect == UINT8_C(0)) {
        if (snapshot->x_request.address_word !=
            (uint32_t)spec->pair.smem_dma7) {
            return false;
        }
    }
    if (snapshot->y_request.address_word !=
        (uint32_t)spec->pair.program_pmad) {
        return false;
    }
    return ti_c54x_memory_instruction_data_program_snapshot_commits_match_spec(
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
ti_c54x_memory_instruction_data_program_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state)
{
    const ti_c54x_dual_memory_address_snapshot *snapshot;
    const ti_c54x_op_memory_address_spec *spec;
    uint16_t selected_ar;

    if (candidate == NULL || state == NULL ||
        !ti_c54x_memory_instruction_data_program_candidate_shape_is_valid(
            &candidate->dual_address_snapshot,
            &candidate->effect.payload.memory) ||
        !pc_candidate_matches_live(&candidate->pc_effect, state)) {
        return false;
    }
    snapshot = &candidate->dual_address_snapshot;
    spec = &candidate->effect.payload.memory.address_spec;
    /* The direct leg and the absolute PROGRAM leg observe no live
     * state; only the indirect Smem leg rechecks its source AR. */
    if (spec->pair.indirect == UINT8_C(0)) {
        return true;
    }
    if (ti_c54x_arch_state_read_ar(state, spec->pair.smem_arf,
                                   &selected_ar) !=
            TI_C54X_ARCH_STATE_OK ||
        selected_ar != snapshot->x_request.address_word) {
        return false;
    }
    return true;
}
