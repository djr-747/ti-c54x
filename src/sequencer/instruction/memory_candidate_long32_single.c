/* Module contract
 *
 * Owns: structural and live-state validation of one copied
 * LONG32_SINGLE DATA address resolution against its memory
 * effect; see memory_candidate_long32_single.h.
 * Does not own: other address kinds, transaction lifecycle,
 * endpoint I/O, architectural mutation, effect completion,
 * opcode dispatch, timing, observers, diagnostics, allocation,
 * or callbacks.
 */

#include "memory_candidate_long32_single.h"

#include <stddef.h>
#include <stdint.h>

#include "address/data_operand.h"
#include "address/indirect_single.h"
#include "bus/request/word16_request.h"
#include "isa/ops/memory/memory_effect.h"
#include "sequencer/instruction/memory_candidate_direct.h"
#include "sequencer/instruction/memory_instruction_transaction.h"
#include "sequencer/memory/address_resolution.h"
#include "state/arch_state.h"

bool
ti_c54x_memory_candidate_long32_single_shape_is_valid(
    const ti_c54x_memory_address_resolution *resolution,
    const ti_c54x_op_memory_effect *effect)
{
    const ti_c54x_address_effect *address_effect;
    const ti_c54x_sequencer_address_commit *address_commit;

    if (resolution == NULL || effect == NULL ||
        resolution->address_result.status != TI_C54X_ADDRESS_STATUS_OK ||
        resolution->request.space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        resolution->request.access != effect->access ||
        resolution->request.address_word !=
            resolution->address_result.effect.operand.first_word ||
        !ti_c54x_bus_word16_request_is_valid(&resolution->request)) {
        return false;
    }
    address_effect = &resolution->address_result.effect;
    address_commit = &resolution->address_commit;

    if (effect->address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE) {
        const ti_c54x_address_source_ar *const source =
            &address_effect->source.value.ar;

        /* LONG32 legs carry canonical MSW/LSW pairs; re-derive the
         * pairing here so a forged operand cannot smuggle a split. */
        if (address_effect->operand.width !=
                TI_C54X_DATA_OPERAND_WIDTH_LONG32 ||
            !ti_c54x_data_operand_is_valid(&address_effect->operand) ||
            address_effect->operand.first_word !=
                resolution->request.address_word) {
            return false;
        }
        if (effect->address_spec.long32_single.indirect == UINT8_C(0)) {
            return address_effect->source.kind ==
                       TI_C54X_ADDRESS_SOURCE_DIRECT_PAGE0 &&
                   address_effect->source.value.direct_page0.dma7 ==
                       effect->address_spec.long32_single.dma7 &&
                   address_effect->state_policy ==
                       TI_C54X_ADDRESS_STATE_POLICY_NONE &&
                   address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_NONE &&
                   address_commit->ar_index == UINT8_C(0) &&
                   address_commit->value_after == UINT16_C(0);
        }
        if (address_effect->source.kind != TI_C54X_ADDRESS_SOURCE_AR ||
            source->index != effect->address_spec.long32_single.arf ||
            address_effect->state_policy !=
                TI_C54X_ADDRESS_STATE_POLICY_CMPT0_ARP0) {
            return false;
        }
        /* The resolved effect does not retain its originating mode
         * discriminator, so recheck the two-word mode contract
         * against the memory effect. */
        switch (effect->address_spec.long32_single.mode) {
        case TI_C54X_INDIRECT_SINGLE_MOD0:
            return address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE &&
                   address_effect->update.ar_index == UINT8_C(0) &&
                   address_effect->update.value_before == UINT16_C(0) &&
                   address_effect->update.value_after == UINT16_C(0) &&
                   address_effect->operand.first_word ==
                       source->value_before &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_NONE &&
                   address_commit->ar_index == UINT8_C(0) &&
                   address_commit->value_after == UINT16_C(0);
        case TI_C54X_INDIRECT_SINGLE_MOD1:
            /* LONG32 MOD1 is a checked post-access decrement by two.
             * The resolver rejects the two-word lower boundary, so
             * proving the arithmetic here cannot wrap a forged
             * pending candidate. */
            return address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
                   address_effect->operand.first_word ==
                       source->value_before &&
                   address_effect->update.ar_index ==
                       effect->address_spec.long32_single.arf &&
                   address_effect->update.value_before ==
                       source->value_before &&
                   source->value_before >= UINT16_C(2) &&
                   address_effect->update.value_after ==
                       (uint16_t)(source->value_before - UINT16_C(2)) &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
                   address_commit->ar_index ==
                       effect->address_spec.long32_single.arf &&
                   address_commit->value_after ==
                       address_effect->update.value_after;
        case TI_C54X_INDIRECT_SINGLE_MOD2:
            /* LONG32 MOD2 is a checked post-access increment by two.
             * The resolver rejects the two-word upper boundary. */
            return address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
                   address_effect->operand.first_word ==
                       source->value_before &&
                   address_effect->update.ar_index ==
                       effect->address_spec.long32_single.arf &&
                   address_effect->update.value_before ==
                       source->value_before &&
                   source->value_before <= UINT16_C(0xFFFD) &&
                   address_effect->update.value_after ==
                       (uint16_t)(source->value_before + UINT16_C(2)) &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
                   address_commit->ar_index ==
                       effect->address_spec.long32_single.arf &&
                   address_commit->value_after ==
                       address_effect->update.value_after;
        case TI_C54X_INDIRECT_SINGLE_MOD3:
            /* LONG32 MOD3 stays write-only with a pre-access
             * increment by two. */
            return effect->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
                   address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS &&
                   address_effect->update.ar_index ==
                       effect->address_spec.long32_single.arf &&
                   address_effect->update.value_before ==
                       source->value_before &&
                   source->value_before <= UINT16_C(0xFFFD) &&
                   address_effect->update.value_after ==
                       (uint16_t)(source->value_before + UINT16_C(2)) &&
                   address_effect->operand.first_word ==
                       address_effect->update.value_after &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
                   address_commit->ar_index ==
                       effect->address_spec.long32_single.arf &&
                   address_commit->value_after ==
                       address_effect->update.value_after;
        default:
            return false;
        }
    }
    return false;
}

bool
ti_c54x_memory_candidate_long32_single_direct_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const struct ti_c54x_arch_state *state)
{
    const ti_c54x_address_effect *address_effect;

    if (candidate == NULL || state == NULL) {
        return false;
    }
    address_effect = &candidate->address_resolution.address_result.effect;
    /* Page-0 direct observes no state: no CMPT/ARP/CPL/DP read, so the
     * generic CMPT=0/ARP=0 gate does not apply. The NONE policy and
     * the dma echo re-prove the frozen shape; only the sequential PC
     * is live. The echo rides the LONG32_SINGLE arm: the DIRECT
     * alias slot overlays reserved storage and reads back zero. */
    return candidate->effect.payload.memory.address_spec.kind ==
               TI_C54X_OP_MEMORY_ADDRESS_LONG32_SINGLE &&
           address_effect->source.kind ==
               TI_C54X_ADDRESS_SOURCE_DIRECT_PAGE0 &&
           address_effect->state_policy ==
               TI_C54X_ADDRESS_STATE_POLICY_NONE &&
           address_effect->source.value.direct_page0.dma7 ==
               candidate->effect.payload.memory.address_spec.long32_single
                   .dma7 &&
           ti_c54x_memory_candidate_pc_matches_live(&candidate->pc_effect,
                                                    state);
}
