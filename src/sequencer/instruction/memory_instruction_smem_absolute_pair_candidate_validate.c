/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied
 * Smem-to-absolute pair candidates; see
 * memory_instruction_smem_absolute_pair_candidate_validate.h.
 * Does not own: transaction lifecycle, endpoint traffic,
 * architectural mutation, effect completion, opcode dispatch,
 * timing, observers, diagnostics, allocation, or global state.
 * Inputs are borrowed for one call.
 */

#include "memory_instruction_smem_absolute_pair_candidate_validate.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/ops/memory/memory_effect.h"
#include "bus/request/word16_request.h"
#include "sequencer/instruction/memory_candidate_absolute_data.h"
#include "sequencer/instruction/memory_candidate_direct.h"
#include "sequencer/instruction/memory_candidate_single_leg.h"
#include "sequencer/instruction/memory_instruction_smem_absolute_pair.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "state/arch_state.h"

bool
ti_c54x_memory_instruction_smem_absolute_pair_candidate_shape_is_valid(
    const struct ti_c54x_memory_instruction_candidate *candidate)
{
    ti_c54x_op_memory_effect data_effect;
    ti_c54x_op_memory_effect dmad_effect;
    ti_c54x_op_memory_address_spec data_spec = {0};
    ti_c54x_op_memory_address_spec dmad_spec = {0};
    const ti_c54x_smem_absolute_pair_address_snapshot *snapshot;

    if (candidate == NULL ||
        candidate->effect.kind != TI_C54X_OP_EFFECT_MEMORY ||
        candidate->effect.payload.memory.address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PAIR_WORD16) {
        return false;
    }
    /* Both legs ride the pair snapshot, not the candidate's
     * generic address resolution: that slot shares union storage
     * with the snapshot. Re-derive both leg specs from the frozen
     * arm and input, then validate each leg against its
     * resolution. The Smem DATA leg runs inverse to the effect
     * access (MVDK reads Smem while the effect completes
     * WRITE). */
    data_effect = candidate->effect.payload.memory;
    data_effect.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    if (!ti_c54x_memory_instruction_smem_absolute_pair_synthesize_data_spec(
            &candidate->effect.payload.memory.address_spec,
            &candidate->input, &data_spec)) {
        return false;
    }
    data_effect.address_spec = data_spec;
    if (!ti_c54x_memory_candidate_single_leg_shape_is_valid(
            &candidate->smem_absolute_address_snapshot.data_resolution,
            &data_effect)) {
        return false;
    }
    if (!ti_c54x_memory_instruction_smem_absolute_pair_synthesize_dmad_spec(
            &candidate->effect.payload.memory.address_spec,
            &dmad_spec)) {
        return false;
    }
    dmad_effect = candidate->effect.payload.memory;
    dmad_effect.address_spec = dmad_spec;
    if (!ti_c54x_memory_candidate_absolute_data_shape_is_valid(
            &candidate->smem_absolute_address_snapshot.dmad_resolution,
            &dmad_effect)) {
        return false;
    }
    /* The dmad WRITE template stays zero until the observed Smem
     * word replaces it at second-leg issue; the snapshot pins
     * the planned address, never the value. */
    snapshot = &candidate->smem_absolute_address_snapshot;
    if (snapshot->second_leg_issued > UINT8_C(1) ||
        snapshot->dmad_resolution.request.write_value_word !=
            UINT16_C(0)) {
        return false;
    }
    return true;
}

bool
ti_c54x_memory_instruction_smem_absolute_pair_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state)
{
    const ti_c54x_address_effect *dmad_effect;
    uint16_t dmad;

    if (candidate == NULL || state == NULL ||
        candidate->effect.kind != TI_C54X_OP_EFFECT_MEMORY ||
        candidate->effect.payload.memory.address_spec.kind !=
            TI_C54X_OP_MEMORY_ADDRESS_SMEM_ABSOLUTE_PAIR_WORD16) {
        return false;
    }
    /* The dmad leg observes no state: re-derive the absolute echo
     * from the frozen arm and pin the sequential PC. */
    dmad = candidate->effect.payload.memory.address_spec.smem_absolute
               .dmad;
    dmad_effect =
        &candidate->smem_absolute_address_snapshot.dmad_resolution
             .address_result.effect;
    if (dmad_effect->operand.first_word != dmad ||
        candidate->smem_absolute_address_snapshot.dmad_resolution.request
                .address_word != (uint32_t)dmad ||
        !ti_c54x_memory_candidate_pc_matches_live(&candidate->pc_effect,
                                                  state)) {
        return false;
    }
    return ti_c54x_memory_instruction_smem_absolute_pair_data_matches_live(
        candidate, state,
        &candidate->smem_absolute_address_snapshot.data_resolution);
}

bool
ti_c54x_memory_instruction_smem_absolute_pair_data_matches_live(
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
        if (arm->smem_absolute.indirect != UINT8_C(0) ||
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
        arm->smem_absolute.indirect != UINT8_C(1)) {
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
        arm->smem_absolute.smem_arf) {
        return false;
    }
    if (ti_c54x_arch_state_read_ar(
            state, address_effect->source.value.ar.index,
            &selected_ar) != TI_C54X_ARCH_STATE_OK) {
        return false;
    }
    return selected_ar == address_effect->source.value.ar.value_before;
}
