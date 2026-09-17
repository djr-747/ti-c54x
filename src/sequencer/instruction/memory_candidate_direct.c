/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied page-0 and
 * DP-relative direct root memory instruction candidates, including the
 * frozen dma/page echo, the concatenated operand, the NONE/CPL0 policy,
 * and the live CPL==0 and DP==dp9 re-reads. Also owns the shared
 * sequential-PC liveness check used by the remaining candidate arms.
 * Does not own: transaction lifecycle, endpoint I/O, architectural
 * mutation, effect completion, opcode dispatch, timing, observers, or
 * diagnostics. Candidate and state arguments are borrowed for one
 * validation call; no pointer is retained and no state is mutated.
 */

#include "memory_candidate_direct.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "isa/ops/memory/memory_effect.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/memory/address_resolution.h"
#include "sequencer/program_pc_bridge.h"
#include "state/arch_state.h"

bool
ti_c54x_memory_candidate_pc_matches_live(
    const ti_c54x_program_pc_bridge_effect *pc,
    const ti_c54x_arch_state *state)
{
    ti_c54x_program_pc_bridge_effect copy;
    uint32_t live_pc;

    if (pc == NULL || state == NULL || pc->public_pc_word_before !=
                                          state->pc_word) {
        return false;
    }
    copy = *pc;
    live_pc = state->pc_word;
    return ti_c54x_program_pc_bridge_apply(&live_pc, &copy) ==
               TI_C54X_PROGRAM_PC_BRIDGE_OK &&
           live_pc == pc->public_pc_word_after;
}

bool
ti_c54x_memory_candidate_direct_shape_is_valid(
    const struct ti_c54x_memory_address_resolution *resolution,
    const struct ti_c54x_op_memory_effect *effect)
{
    const ti_c54x_address_effect *address_effect;
    const ti_c54x_sequencer_address_commit *address_commit;

    if (resolution == NULL || effect == NULL) {
        return false;
    }
    address_effect = &resolution->address_result.effect;
    address_commit = &resolution->address_commit;

    if (effect->address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16) {
        /* Page-0 direct carries no update and no policy: the dma echo
         * and the zero-extended operand prove the forged-candidate
         * contract with no state observation. */
        return address_effect->source.kind ==
                   TI_C54X_ADDRESS_SOURCE_DIRECT_PAGE0 &&
               address_effect->source.value.direct_page0.dma7 ==
                   effect->address_spec.direct_dma7 &&
               address_effect->operand.first_word ==
                   effect->address_spec.direct_dma7 &&
               address_effect->state_policy ==
                   TI_C54X_ADDRESS_STATE_POLICY_NONE &&
               address_effect->update.target ==
                   TI_C54X_ADDRESS_UPDATE_NONE &&
               address_effect->update.phase ==
                   TI_C54X_ADDRESS_UPDATE_PHASE_NONE &&
               address_effect->update.ar_index == UINT8_C(0) &&
               address_effect->update.value_before == UINT16_C(0) &&
               address_effect->update.value_after == UINT16_C(0) &&
               address_commit->target == TI_C54X_ADDRESS_UPDATE_NONE &&
               address_commit->ar_index == UINT8_C(0) &&
               address_commit->value_after == UINT16_C(0);
    }

    if (effect->address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_DIRECT_DP_WORD16) {
        const uint16_t expected_first_word = (uint16_t)(
            ((uint16_t)(effect->address_spec.long_offset_word << 7U)) |
            effect->address_spec.direct_dma7);
        /* DP-relative direct carries no update: the frozen DP page and
         * offset echo plus the concatenated operand prove the
         * forged-candidate contract with no live state observation
         * (DP unknownness is gated at the leaf). CPL0 policy only. */
        return address_effect->source.kind ==
                   TI_C54X_ADDRESS_SOURCE_DIRECT_DP &&
               address_effect->source.value.direct_dp.dp9 ==
                   effect->address_spec.long_offset_word &&
               address_effect->source.value.direct_dp.dma7 ==
                   effect->address_spec.direct_dma7 &&
               address_effect->operand.first_word == expected_first_word &&
               address_effect->state_policy ==
                   TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0 &&
               address_effect->update.target ==
                   TI_C54X_ADDRESS_UPDATE_NONE &&
               address_effect->update.phase ==
                   TI_C54X_ADDRESS_UPDATE_PHASE_NONE &&
               address_effect->update.ar_index == UINT8_C(0) &&
               address_effect->update.value_before == UINT16_C(0) &&
               address_effect->update.value_after == UINT16_C(0) &&
               address_commit->target == TI_C54X_ADDRESS_UPDATE_NONE &&
               address_commit->ar_index == UINT8_C(0) &&
               address_commit->value_after == UINT16_C(0);
    }

    return false;
}

bool
ti_c54x_memory_candidate_direct_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const ti_c54x_arch_state *state)
{
    const ti_c54x_address_effect *address_effect;

    if (candidate == NULL || state == NULL) {
        return false;
    }
    address_effect = &candidate->address_resolution.address_result.effect;

    if (address_effect->source.kind ==
        TI_C54X_ADDRESS_SOURCE_DIRECT_PAGE0) {
        /* Page-0 direct observes no state: no CMPT/ARP/CPL/DP read,
         * so the generic CMPT=0/ARP=0 gate does not apply. The NONE
         * policy and the dma echo re-prove the frozen shape; only the
         * sequential PC is live. */
        return address_effect->state_policy ==
                   TI_C54X_ADDRESS_STATE_POLICY_NONE &&
               address_effect->source.value.direct_page0.dma7 ==
                   candidate->effect.payload.memory.address_spec
                       .direct_dma7 &&
               ti_c54x_memory_candidate_pc_matches_live(
                   &candidate->pc_effect, state);
    }

    if (address_effect->source.kind == TI_C54X_ADDRESS_SOURCE_DIRECT_DP) {
        uint8_t cpl = UINT8_C(0);
        uint16_t dp = UINT16_C(0);

        /* DP-relative direct observes CPL and DP only: CMPT/ARP are
         * not inputs, so the generic gate does not apply. The CPL0
         * policy plus the frozen page/offset echo re-prove the shape;
         * CPL==0 and DP==dp9 are re-read live because either can
         * change across a WAIT resume. */
        if (address_effect->state_policy !=
                TI_C54X_ADDRESS_STATE_POLICY_DIRECT_CPL0 ||
            address_effect->source.value.direct_dp.dp9 !=
                candidate->effect.payload.memory.address_spec
                    .long_offset_word ||
            address_effect->source.value.direct_dp.dma7 !=
                candidate->effect.payload.memory.address_spec
                    .direct_dma7 ||
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

    return false;
}
