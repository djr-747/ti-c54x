/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied stack-pair
 * candidates; see
 * memory_instruction_stack_pair_candidate_validate.h. Direct
 * page-0 and MOD-gated single-indirect DATA legs share the
 * snapshot carrier.
 * Does not own: transaction lifecycle, endpoint traffic,
 * architectural mutation, effect completion, opcode dispatch,
 * timing, observers, diagnostics, allocation, or global state.
 * Inputs are borrowed for one call.
 */

#include "memory_instruction_stack_pair_candidate_validate.h"

#include <stddef.h>
#include <stdint.h>

#include "isa/ops/memory/memory_effect.h"
#include "bus/request/word16_request.h"
#include "sequencer/instruction/memory_candidate_direct.h"
#include "sequencer/instruction/memory_candidate_single_leg.h"
#include "sequencer/instruction/memory_instruction_stack_pair.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "state/arch_state.h"

static bool
arm_kind_is_pair(ti_c54x_op_memory_address_kind kind)
{
    return kind ==
               TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_PAIR_WORD16 ||
           kind == TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_PAIR_WORD16;
}

bool
ti_c54x_memory_instruction_stack_pair_candidate_shape_is_valid(
    const struct ti_c54x_memory_instruction_candidate *candidate)
{
    ti_c54x_op_memory_effect data_effect;
    ti_c54x_op_memory_effect stack_effect;
    ti_c54x_op_memory_address_spec data_spec = {0};
    ti_c54x_op_memory_address_kind stack_kind =
        TI_C54X_OP_MEMORY_ADDRESS_INVALID;

    if (candidate == NULL ||
        candidate->effect.kind != TI_C54X_OP_EFFECT_MEMORY ||
        !arm_kind_is_pair(
            candidate->effect.payload.memory.address_spec.kind)) {
        return false;
    }
    /* Both legs ride the pair snapshot, not the candidate's generic
     * address resolution: that slot shares union storage with the
     * snapshot. Re-derive both leg specs from the frozen arm and
     * validate each leg against its synthesized spec. */
    data_effect = candidate->effect.payload.memory;
    stack_effect = candidate->effect.payload.memory;
    /* The Smem DATA leg runs inverse to the effect access (the
     * access names the stack-leg direction): PSHD reads Smem,
     * POPD writes it. */
    data_effect.access =
        (candidate->effect.payload.memory.access ==
                 TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE)
            ? TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
            : TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    if (!ti_c54x_memory_instruction_stack_pair_synthesize_data_spec(
            &candidate->effect.payload.memory.address_spec,
            &data_spec)) {
        return false;
    }
    if (!ti_c54x_memory_instruction_stack_pair_synthesize_stack_spec(
            &candidate->effect.payload.memory.address_spec,
            &stack_kind)) {
        return false;
    }
    data_effect.address_spec = data_spec;
    stack_effect.address_spec.kind = stack_kind;
    if (!ti_c54x_memory_candidate_single_leg_shape_is_valid(
            &candidate->stack_pair_address_snapshot.data_resolution,
            &data_effect)) {
        return false;
    }
    if (!ti_c54x_memory_candidate_single_leg_shape_is_valid(
            &candidate->stack_pair_address_snapshot.stack_resolution,
            &stack_effect)) {
        return false;
    }
    if (candidate->stack_pair_address_snapshot.second_leg_issued >
        UINT8_C(1)) {
        return false;
    }
    return true;
}

bool
ti_c54x_memory_instruction_stack_pair_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state)
{
    const ti_c54x_address_effect *stack_effect;
    uint16_t live_sp = UINT16_C(0);

    if (candidate == NULL || state == NULL ||
        candidate->effect.kind != TI_C54X_OP_EFFECT_MEMORY ||
        !arm_kind_is_pair(
            candidate->effect.payload.memory.address_spec.kind)) {
        return false;
    }
    /* Pushes and pops observe no CMPT/ARP state: only the live PC
     * anchor plus the frozen SP snapshot (mirroring the single-leg
     * stack arm; pair kinds never load RTN). The shape check above
     * already pins the operand/update contract. */
    stack_effect =
        &candidate->stack_pair_address_snapshot.stack_resolution
             .address_result.effect;
    if (!ti_c54x_memory_candidate_pc_matches_live(&candidate->pc_effect,
                                                  state) ||
        stack_effect->source.kind != TI_C54X_ADDRESS_SOURCE_STACK_PUSH) {
        return false;
    }
    if (ti_c54x_arch_state_read_sp(state, &live_sp) !=
        TI_C54X_ARCH_STATE_OK) {
        return false;
    }
    if (live_sp != stack_effect->source.value.stack_push.sp_before) {
        return false;
    }
    return ti_c54x_memory_instruction_stack_pair_data_matches_live(
        candidate, state,
        &candidate->stack_pair_address_snapshot.data_resolution);
}

bool
ti_c54x_memory_instruction_stack_pair_data_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state,
    const ti_c54x_memory_address_resolution *resolution)
{
    const ti_c54x_address_effect *address_effect;
    const ti_c54x_op_memory_address_spec *arm;
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
        TI_C54X_ADDRESS_SOURCE_DIRECT_PAGE0) {
        /* Direct page-0 DATA leg: dma below 0x20 routes to the
         * MMR file, the rest reaches DP0 endpoint RAM
         * (DIRECT_WORD16 contract). Page-0 direct observes no
         * state, so liveness binds the resolution's dma echo
         * against the frozen pair arm (not the generic
         * direct-dma7 union offset, which a pair arm does not
         * populate) plus the sequential PC. */
        if (address_effect->state_policy !=
                TI_C54X_ADDRESS_STATE_POLICY_NONE ||
            address_effect->source.value.direct_page0.dma7 !=
                arm->stack_pair.smem_dma7) {
            return false;
        }
        return ti_c54x_memory_candidate_pc_matches_live(
            &candidate->pc_effect, state);
    }
    if (address_effect->source.kind != TI_C54X_ADDRESS_SOURCE_AR ||
        arm->stack_pair.indirect != UINT8_C(1)) {
        return false;
    }
    /* Single-indirect DATA leg (classes 16B/17B): mirror the
     * generic CMPT0/ARP0 indirect tail. The mode is structural
     * (pinned once by shape against the frozen arm); liveness
     * re-pins the CMPT/ARP plan, the sequential PC, the AR
     * selector echo, and the frozen AR value. */
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
        arm->stack_pair.smem_arf) {
        return false;
    }
    if (ti_c54x_arch_state_read_ar(
            state, address_effect->source.value.ar.index,
            &selected_ar) != TI_C54X_ARCH_STATE_OK) {
        return false;
    }
    return selected_ar == address_effect->source.value.ar.value_before;
}
