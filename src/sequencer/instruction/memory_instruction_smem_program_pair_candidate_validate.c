/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied
 * Smem-to-program pair candidates; see
 * memory_instruction_smem_program_pair_candidate_validate.h.
 * Does not own: transaction lifecycle, endpoint traffic,
 * architectural mutation, effect completion, opcode dispatch,
 * timing, observers, diagnostics, allocation, or global state.
 * Inputs are borrowed for one call.
 */

#include "memory_instruction_smem_program_pair_candidate_validate.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "isa/ops/memory/memory_effect.h"
#include "bus/request/word16_request.h"
#include "sequencer/instruction/memory_candidate_direct.h"
#include "sequencer/instruction/memory_candidate_single_leg.h"
#include "sequencer/instruction/memory_instruction_smem_program_pair.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "state/arch_state.h"

bool
ti_c54x_memory_instruction_smem_program_pair_candidate_shape_is_valid(
    const struct ti_c54x_memory_instruction_candidate *candidate)
{
    ti_c54x_op_memory_effect smem_effect;
    ti_c54x_op_memory_address_spec smem_spec = {0};
    const ti_c54x_smem_program_pair_address_snapshot *snapshot;
    const ti_c54x_bus_word16_request *program_request;
    uint16_t pmad;

    if (candidate == NULL ||
        candidate->effect.kind != TI_C54X_OP_EFFECT_MEMORY ||
        candidate->effect.payload.memory.address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16) {
        return false;
    }
    /* Both legs ride the pair snapshot, not the candidate's
     * generic address resolution: that slot shares union storage
     * with the snapshot. Re-derive the Smem-leg spec from the
     * frozen arm and input and validate it against the Smem
     * resolution; the PROGRAM leg is stateless, so echo the
     * built request. The Smem leg runs at the effect access
     * (the access names the Smem-leg direction: MVPD writes
     * Smem, MVDP reads Smem). */
    smem_effect = candidate->effect.payload.memory;
    if (!ti_c54x_memory_instruction_smem_program_pair_synthesize_smem_spec(
            &candidate->effect.payload.memory.address_spec,
            &candidate->input, &smem_spec)) {
        return false;
    }
    smem_effect.address_spec = smem_spec;
    if (!ti_c54x_memory_candidate_single_leg_shape_is_valid(
            &candidate->smem_program_address_snapshot.smem_resolution,
            &smem_effect)) {
        return false;
    }
    /* The PROGRAM request template stays zero until the observed
     * first-leg word replaces it at second-leg issue; the
     * snapshot pins the planned space, address, and direction,
     * never the value. */
    snapshot = &candidate->smem_program_address_snapshot;
    program_request = &snapshot->program_request;
    pmad = candidate->effect.payload.memory.address_spec.smem_program
               .pmad;
    if (snapshot->second_leg_issued > UINT8_C(1) ||
        program_request->space !=
            TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM ||
        program_request->address_word != (uint32_t)pmad ||
        program_request->write_value_word != UINT16_C(0)) {
        return false;
    }
    if (candidate->effect.payload.memory.access ==
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        /* MVPD reads PROGRAM first. */
        if (program_request->access !=
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
            return false;
        }
    } else if (program_request->access !=
               TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        /* MVDP writes PROGRAM second. */
        return false;
    }
    return ti_c54x_bus_word16_request_is_valid(program_request);
}

bool
ti_c54x_memory_instruction_smem_program_pair_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state)
{
    uint16_t pmad;

    if (candidate == NULL || state == NULL ||
        candidate->effect.kind != TI_C54X_OP_EFFECT_MEMORY ||
        candidate->effect.payload.memory.address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_SMEM_PROGRAM_PAIR_WORD16) {
        return false;
    }
    /* The PROGRAM leg observes no state: echo the frozen pmad
     * against the built request and pin the sequential PC. */
    pmad = candidate->effect.payload.memory.address_spec.smem_program
               .pmad;
    if (candidate->smem_program_address_snapshot.program_request
                .address_word != (uint32_t)pmad ||
        candidate->smem_program_address_snapshot.program_request.space !=
            TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM ||
        !ti_c54x_memory_candidate_pc_matches_live(&candidate->pc_effect,
                                                  state)) {
        return false;
    }
    return ti_c54x_memory_instruction_smem_program_pair_smem_matches_live(
        candidate, state,
        &candidate->smem_program_address_snapshot.smem_resolution);
}

bool
ti_c54x_memory_instruction_smem_program_pair_smem_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state,
    const ti_c54x_memory_address_resolution *resolution)
{
    const ti_c54x_address_effect *address_effect;
    const ti_c54x_op_memory_address_spec *arm;
    uint8_t cpl = UINT8_C(0);
    uint16_t dp = UINT16_C(0);
    uint8_t cmpt = UINT8_C(0);
    uint8_t arp = UINT8_C(0);
    uint16_t selected_ar = UINT16_C(0);

    if (candidate == NULL || state == NULL || resolution == NULL ||
        candidate->effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    address_effect = &resolution->address_result.effect;
    arm = &candidate->effect.payload.memory.address_spec;
    if (address_effect->source.kind ==
        TI_C54X_ADDRESS_SOURCE_DIRECT_DP) {
        /* DP-relative direct DATA leg: the arm carries no DP page
         * echo (the page rides the frozen input and shape rebinds
         * it on every check), so liveness pins the CPL0 policy
         * plus live CPL/DP against the leg's own page and then
         * the sequential PC (PORT precedent). */
        if (arm->smem_program.indirect != UINT8_C(0) ||
            address_effect->state_policy !=
                TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0 ||
            ti_c54x_arch_state_read_cpl(state, &cpl) !=
                TI_C54X_ARCH_STATE_OK ||
            cpl != UINT8_C(0) ||
            ti_c54x_arch_state_read_dp(state, &dp) !=
                TI_C54X_ARCH_STATE_OK ||
            dp != address_effect->source.value.direct_dp.dp9) {
            return false;
        }
        return ti_c54x_memory_candidate_pc_matches_live(
            &candidate->pc_effect, state);
    }
    if (address_effect->source.kind != TI_C54X_ADDRESS_SOURCE_AR ||
        arm->smem_program.indirect != UINT8_C(1)) {
        return false;
    }
    /* Single-indirect DATA leg: mirror the generic CMPT0/ARP0
     * indirect tail. The mode is structural (pinned once by shape
     * against the frozen arm); liveness re-pins the CMPT/ARP plan,
     * the sequential PC, the AR selector echo, and the frozen AR
     * value. */
    if (ti_c54x_arch_state_read_cmpt(state, &cmpt) !=
            TI_C54X_ARCH_STATE_OK ||
        cmpt != UINT8_C(0) ||
        ti_c54x_arch_state_read_arp(state, &arp) !=
            TI_C54X_ARCH_STATE_OK ||
        arp != UINT8_C(0) ||
        address_effect->state_policy !=
            TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) {
        return false;
    }
    if (!ti_c54x_memory_candidate_pc_matches_live(&candidate->pc_effect,
                                                  state)) {
        return false;
    }
    if (address_effect->source.value.ar.index !=
        arm->smem_program.smem_arf) {
        return false;
    }
    if (ti_c54x_arch_state_read_ar(
            state, address_effect->source.value.ar.index,
            &selected_ar) != TI_C54X_ARCH_STATE_OK) {
        return false;
    }
    return selected_ar == address_effect->source.value.ar.value_before;
}
