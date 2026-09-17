/*
 * Module contract
 *
 * Owns: structural validation of one copied single-leg DATA address
 * resolution against its memory effect; see memory_candidate_single_leg.h.
 * Does not own: transaction lifecycle, endpoint I/O, architectural
 * mutation, effect completion, opcode dispatch, timing, observers,
 * diagnostics, allocation, or global state. Inputs are borrowed for
 * one call.
 */

#include "memory_candidate_single_leg.h"

#include <stddef.h>
#include <stdint.h>

#include "address/indirect_circular.h"
#include "address/indirect_dual_mod3.h"
#include "address/indirect_long_offset.h"
#include "address/indirect_reverse_carry.h"
#include "bus/request/word16_request.h"
#include "sequencer/instruction/memory_candidate_direct.h"
#include "sequencer/memory/address_resolution.h"

bool
ti_c54x_memory_candidate_single_leg_shape_is_valid(
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
        TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32) {
        const ti_c54x_address_source_ar_long_offset *const source =
            &address_effect->source.value.ar_long_offset;
        return address_effect->operand.width ==
                   TI_C54X_DATA_OPERAND_WIDTH_LONG32 &&
               ti_c54x_indirect_long_offset_effect_is_valid(address_effect) &&
               address_effect->source.kind ==
                   TI_C54X_ADDRESS_SOURCE_AR_LONG_OFFSET &&
               source->index == effect->address_spec.arf &&
               source->long_offset_word ==
                   effect->address_spec.long_offset_word &&
               address_effect->operand.first_word ==
                   resolution->request.address_word &&
               address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
               address_commit->ar_index == source->index &&
               address_commit->value_after ==
                   address_effect->update.value_after;
    }
    if (address_effect->operand.width != TI_C54X_DATA_OPERAND_WIDTH_WORD16) {
        return false;
    }

    if (effect->address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_SINGLE_INDIRECT_WORD16) {
        if (address_effect->source.kind != TI_C54X_ADDRESS_SOURCE_AR ||
            address_effect->source.value.ar.index !=
                effect->address_spec.arf) {
            return false;
        }
        /* The resolved address effect does not retain its originating mode
         * discriminator, so recheck the mode-specific operand/update
         * contract against the memory effect. This keeps forged candidates
         * from reaching the endpoint while leaving arithmetic to the address
         * planner. */
        switch (effect->address_spec.mode) {
        case TI_C54X_INDIRECT_SINGLE_MOD0:
            return address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE &&
                   address_effect->update.ar_index == UINT8_C(0) &&
                   address_effect->update.value_before == UINT16_C(0) &&
                   address_effect->update.value_after == UINT16_C(0) &&
                   address_effect->operand.first_word ==
                       address_effect->source.value.ar.value_before &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_NONE &&
                   address_commit->ar_index == UINT8_C(0) &&
                   address_commit->value_after == UINT16_C(0);
        case TI_C54X_INDIRECT_SINGLE_MOD1:
            /* Word16 MOD1 is a checked post-access decrement.  The
             * resolver rejects the zero boundary, so proving the arithmetic
             * here cannot wrap a forged pending candidate. */
            return address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
                   address_effect->operand.first_word ==
                       address_effect->source.value.ar.value_before &&
                   address_effect->update.ar_index == effect->address_spec.arf &&
                   address_effect->update.value_before ==
                       address_effect->source.value.ar.value_before &&
                   address_effect->source.value.ar.value_before >=
                       UINT16_C(1) &&
                   address_effect->update.value_after ==
                       (uint16_t)(address_effect->source.value.ar.value_before -
                                  UINT16_C(1)) &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
                   address_commit->ar_index == effect->address_spec.arf &&
                   address_commit->value_after ==
                       address_effect->update.value_after;
        case TI_C54X_INDIRECT_SINGLE_MOD2:
            /* Word16 MOD2 is a checked post-access increment.  The
             * resolver rejects the 0xffff boundary, so require the exact
             * non-wrapping successor and its source snapshot. */
            return address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
                   address_effect->operand.first_word ==
                       address_effect->source.value.ar.value_before &&
                   address_effect->update.ar_index == effect->address_spec.arf &&
                   address_effect->update.value_before ==
                       address_effect->source.value.ar.value_before &&
                   address_effect->source.value.ar.value_before <
                       UINT16_MAX &&
                   address_effect->update.value_after ==
                       (uint16_t)(address_effect->source.value.ar.value_before +
                                  UINT16_C(1)) &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
                   address_commit->ar_index == effect->address_spec.arf &&
                   address_commit->value_after ==
                       address_effect->update.value_after;
        case TI_C54X_INDIRECT_SINGLE_MOD3:
            return effect->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
                   address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS &&
                   address_effect->update.ar_index == effect->address_spec.arf &&
                   address_effect->update.value_before ==
                       address_effect->source.value.ar.value_before &&
                   address_effect->source.value.ar.value_before <
                       UINT16_MAX &&
                   address_effect->update.value_after ==
                       (uint16_t)(address_effect->source.value.ar.value_before +
                                  UINT16_C(1)) &&
                   address_effect->operand.first_word ==
                       address_effect->update.value_after &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
                   address_commit->ar_index == effect->address_spec.arf &&
                   address_commit->value_after ==
                       address_effect->update.value_after;
        default:
            return false;
        }
    }

    if (effect->address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_INDEXED_AR0_WORD16) {
        const ti_c54x_address_source_ar_indexed *const source =
            &address_effect->source.value.ar_indexed;
        uint16_t expected_after;

        if (address_effect->source.kind !=
                TI_C54X_ADDRESS_SOURCE_AR_INDEXED ||
            source->index != effect->address_spec.arf ||
            address_effect->operand.first_word != source->value_before ||
            address_effect->update.target != TI_C54X_ADDRESS_UPDATE_AR ||
            address_effect->update.phase !=
                TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS ||
            address_effect->update.ar_index != source->index ||
            address_effect->update.value_before != source->value_before ||
            address_commit->target != TI_C54X_ADDRESS_UPDATE_AR ||
            address_commit->ar_index != source->index ||
            address_commit->value_after !=
                address_effect->update.value_after) {
            return false;
        }
        if (effect->address_spec.indexed_mode ==
            TI_C54X_INDIRECT_INDEXED_MOD5) {
            if (source->value_before < source->ar0_value_before) {
                return false;
            }
            expected_after = (uint16_t)(source->value_before -
                                        source->ar0_value_before);
        } else if (effect->address_spec.indexed_mode ==
                   TI_C54X_INDIRECT_INDEXED_MOD6) {
            const uint32_t sum = (uint32_t)source->value_before +
                                  (uint32_t)source->ar0_value_before;
            if (sum > UINT32_C(0xffff)) {
                return false;
            }
            expected_after = (uint16_t)sum;
        } else {
            return false;
        }
        return address_effect->update.value_after == expected_after;
    }

    if (effect->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_DIRECT_WORD16 ||
        effect->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_DIRECT_DP_WORD16) {
        return ti_c54x_memory_candidate_direct_shape_is_valid(resolution,
                                                              effect);
    }

    if (effect->address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_CIRCULAR_WORD16) {
        const ti_c54x_address_source_ar_circular *const source =
            &address_effect->source.value.ar_circular;

        if (!ti_c54x_indirect_circular_effect_is_valid(address_effect) ||
            address_effect->source.kind !=
                TI_C54X_ADDRESS_SOURCE_AR_CIRCULAR ||
            source->index != effect->address_spec.arf ||
            source->modification !=
                (uint8_t)effect->address_spec.circular_mode ||
            address_effect->operand.first_word != source->value_before) {
            return false;
        }
        if (source->bk_value_before == UINT16_C(0)) {
            return address_effect->update.target ==
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
        return address_effect->update.target == TI_C54X_ADDRESS_UPDATE_AR &&
               address_effect->update.phase ==
                   TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
               address_effect->update.ar_index == source->index &&
               address_effect->update.value_before == source->value_before &&
               address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
               address_commit->ar_index == source->index &&
               address_commit->value_after ==
                   address_effect->update.value_after;
    }

    if (effect->address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16_X_ONLY) {
        const ti_c54x_address_source_ar *const source =
            &address_effect->source.value.ar;
        const uint8_t x_ar = effect->address_spec.dual.x_ar;
        const uint8_t x_mode = effect->address_spec.dual.x_mode;

        /* Y self-aliases X by effect contract while the single resolution
         * carries only the X side. Recheck the mode-specific operand/update
         * contract against the memory effect so forged candidates cannot
         * reach the endpoint while leaving arithmetic to the planner. */
        if (effect->address_spec.dual.y_mode != x_mode ||
            effect->address_spec.dual.y_ar != x_ar || x_mode > UINT8_C(3) ||
            x_ar < UINT8_C(2) || x_ar > UINT8_C(5) ||
            effect->address_spec.dual.reserved_word0 != UINT32_C(0) ||
            effect->address_spec.dual.reserved_word1 != UINT32_C(0) ||
            effect->address_spec.dual.reserved_word2 != UINT32_C(0) ||
            effect->address_spec.dual.reserved_word3 != UINT32_C(0) ||
            address_effect->source.kind != TI_C54X_ADDRESS_SOURCE_AR ||
            source->index != x_ar ||
            address_effect->operand.width !=
                TI_C54X_DATA_OPERAND_WIDTH_WORD16 ||
            address_effect->operand.first_word != source->value_before ||
            address_effect->state_policy !=
                TI_C54X_ADDRESS_STATE_POLICY_NONE) {
            return false;
        }
        switch (x_mode) {
        case UINT8_C(0):
            return address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_NONE &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_NONE &&
                   address_effect->update.ar_index == UINT8_C(0) &&
                   address_effect->update.value_before == UINT16_C(0) &&
                   address_effect->update.value_after == UINT16_C(0) &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_NONE &&
                   address_commit->ar_index == UINT8_C(0) &&
                   address_commit->value_after == UINT16_C(0);
        case UINT8_C(1):
            /* Word16 MOD1 is a checked post-access decrement. The
             * resolver rejects the zero boundary, so proving the
             * arithmetic here cannot wrap a forged pending candidate. */
            return address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
                   address_effect->update.ar_index == x_ar &&
                   address_effect->update.value_before ==
                       source->value_before &&
                   source->value_before >= UINT16_C(1) &&
                   address_effect->update.value_after ==
                       (uint16_t)(source->value_before - UINT16_C(1)) &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
                   address_commit->ar_index == x_ar &&
                   address_commit->value_after ==
                       address_effect->update.value_after;
        case UINT8_C(2):
            /* Word16 MOD2 is a checked post-access increment. The
             * resolver rejects the 0xffff boundary, so require the exact
             * non-wrapping successor and its source snapshot. */
            return address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
                   address_effect->update.ar_index == x_ar &&
                   address_effect->update.value_before ==
                       source->value_before &&
                   source->value_before < UINT16_MAX &&
                   address_effect->update.value_after ==
                       (uint16_t)(source->value_before + UINT16_C(1)) &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
                   address_commit->ar_index == x_ar &&
                   address_commit->value_after ==
                       address_effect->update.value_after;
        case UINT8_C(3): {
            /* Word16 MOD3 stages a post-access AR0/BK update recomputed
             * from the resolution shadows; the documented BK=0/AR0=0 case
             * performs no modification. The resolver rejects MOD3
             * boundary violations, so a forged boundary-crossing commit
             * cannot validate here either. */
            uint16_t expected_after = UINT16_C(0);

            if (ti_c54x_indirect_dual_mod3_after(
                    source->value_before, resolution->ar0_shadow,
                    resolution->bk_shadow,
                    &expected_after) != TI_C54X_ADDRESS_STATUS_OK) {
                return false;
            }
            if (resolution->ar0_shadow == UINT16_C(0) &&
                resolution->bk_shadow == UINT16_C(0)) {
                return address_effect->update.target ==
                           TI_C54X_ADDRESS_UPDATE_NONE &&
                       address_effect->update.phase ==
                           TI_C54X_ADDRESS_UPDATE_PHASE_NONE &&
                       address_effect->update.ar_index == UINT8_C(0) &&
                       address_effect->update.value_before == UINT16_C(0) &&
                       address_effect->update.value_after == UINT16_C(0) &&
                       address_commit->target ==
                           TI_C54X_ADDRESS_UPDATE_NONE &&
                       address_commit->ar_index == UINT8_C(0) &&
                       address_commit->value_after == UINT16_C(0);
            }
            return address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
                   address_effect->update.ar_index == x_ar &&
                   address_effect->update.value_before ==
                       source->value_before &&
                   address_effect->update.value_after == expected_after &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
                   address_commit->ar_index == x_ar &&
                   address_commit->value_after ==
                       address_effect->update.value_after;
        }
        default:
            return false;
        }
    }

    if (effect->address_spec.kind ==
        TI_C54X_OP_MEMORY_ADDRESS_REVERSE_CARRY_WORD16) {
        const ti_c54x_address_source_ar_reverse_carry *const source =
            &address_effect->source.value.ar_reverse_carry;

        return ti_c54x_indirect_reverse_carry_effect_is_valid(address_effect) &&
               address_effect->source.kind ==
                   TI_C54X_ADDRESS_SOURCE_AR_REVERSE_CARRY &&
               source->index == effect->address_spec.arf &&
               source->modification ==
                   (uint8_t)effect->address_spec.reverse_carry_mode &&
               address_effect->operand.first_word == source->value_before &&
               address_effect->update.target ==
                   TI_C54X_ADDRESS_UPDATE_AR &&
               address_effect->update.phase ==
                   TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
               address_effect->update.ar_index == source->index &&
               address_effect->update.value_before == source->value_before &&
               address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
               address_commit->ar_index == source->index &&
               address_commit->value_after ==
                   address_effect->update.value_after;
    }

    if (effect->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_WORD16 ||
        effect->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_DELAYED_WORD16 ||
        effect->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_WORD16 ||
        effect->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_PUSH_ACCUMULATOR_DELAYED_WORD16) {
        const ti_c54x_address_source_stack_push *const source =
            &address_effect->source.value.stack_push;
        /* Wrapping TOS arithmetic mirrors the planner exactly, so a forged
         * boundary-crossing commit cannot validate here either. */
        const uint16_t expected_tos =
            (uint16_t)(source->sp_before - UINT16_C(1));

        if (address_effect->source.kind !=
            TI_C54X_ADDRESS_SOURCE_STACK_PUSH) {
            return false;
        }
        return address_effect->operand.first_word == expected_tos &&
               address_effect->update.target ==
                   TI_C54X_ADDRESS_UPDATE_SP &&
               address_effect->update.phase ==
                   TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS &&
               address_effect->update.ar_index == UINT8_C(0) &&
               address_effect->update.value_before == source->sp_before &&
               address_effect->update.value_after == expected_tos &&
               address_commit->target == TI_C54X_ADDRESS_UPDATE_SP &&
               address_commit->ar_index == UINT8_C(0) &&
               address_commit->value_after == expected_tos;
    }

    if (effect->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_WORD16 ||
        effect->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_DELAYED_WORD16 ||
        effect->address_spec.kind ==
            TI_C54X_OP_MEMORY_ADDRESS_STACK_POP_RTN_WORD16) {
        const ti_c54x_address_source_stack_push *const source =
            &address_effect->source.value.stack_push;
        /* Wrapping SP+1 arithmetic mirrors the planner exactly, so a
         * forged boundary-crossing commit cannot validate here either. */
        const uint16_t expected_after =
            (uint16_t)(source->sp_before + UINT16_C(1));

        if (address_effect->source.kind !=
            TI_C54X_ADDRESS_SOURCE_STACK_PUSH) {
            return false;
        }
        return address_effect->operand.first_word == source->sp_before &&
               address_effect->update.target ==
                   TI_C54X_ADDRESS_UPDATE_SP &&
               address_effect->update.phase ==
                   TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
               address_effect->update.ar_index == UINT8_C(0) &&
               address_effect->update.value_before == source->sp_before &&
               address_effect->update.value_after == expected_after &&
               address_commit->target == TI_C54X_ADDRESS_UPDATE_SP &&
               address_commit->ar_index == UINT8_C(0) &&
               address_commit->value_after == expected_after;
    }

    return false;
}
