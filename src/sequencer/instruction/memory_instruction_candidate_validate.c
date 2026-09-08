/*
 * Module contract
 *
 * Owns: structural and live-state validation of copied root memory
 * instruction candidates, including exact address-resolution snapshots for
 * single-indirect and AR0-indexed operands.
 * Does not own: transaction lifecycle, endpoint I/O, architectural mutation,
 * effect completion, opcode dispatch, timing, observers, or diagnostics.
 */

#include "memory_instruction_candidate_validate.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "sequencer/instruction/memory_instruction_transaction.h"

static bool
address_resolution_is_valid(
    const ti_c54x_memory_address_resolution *resolution,
    const ti_c54x_op_memory_effect *effect)
{
    const ti_c54x_address_effect *address_effect;
    const ti_c54x_sequencer_address_commit *address_commit;

    if (resolution == NULL || effect == NULL ||
        resolution->address_result.status != TI_C54X_ADDRESS_STATUS_OK ||
        resolution->address_result.effect.operand.width !=
            TI_C54X_DATA_OPERAND_WIDTH_WORD16 ||
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
                   address_effect->operand.first_word ==
                       address_effect->source.value.ar.value_before &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_NONE &&
                   address_commit->ar_index == UINT8_C(0) &&
                   address_commit->value_after == UINT16_C(0);
        case TI_C54X_INDIRECT_SINGLE_MOD1:
        case TI_C54X_INDIRECT_SINGLE_MOD2:
            return address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_POST_ACCESS &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
                   address_commit->ar_index == effect->address_spec.arf &&
                   address_commit->value_after ==
                       address_effect->update.value_after &&
                   address_commit->value_after !=
                       address_effect->update.value_before;
        case TI_C54X_INDIRECT_SINGLE_MOD3:
            return effect->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
                   address_effect->update.target ==
                       TI_C54X_ADDRESS_UPDATE_AR &&
                   address_effect->update.phase ==
                       TI_C54X_ADDRESS_UPDATE_PHASE_PRE_ACCESS &&
                   address_effect->operand.first_word ==
                       address_effect->update.value_after &&
                   address_commit->target == TI_C54X_ADDRESS_UPDATE_AR &&
                   address_commit->ar_index == effect->address_spec.arf &&
                   address_commit->value_after ==
                       address_effect->update.value_after &&
                   address_commit->value_after !=
                       address_effect->update.value_before;
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

    return false;
}

static bool
address_resolution_matches(
    const ti_c54x_memory_address_resolution *expected,
    const ti_c54x_memory_address_resolution *actual)
{
    const ti_c54x_address_effect *expected_effect;
    const ti_c54x_address_effect *actual_effect;

    if (expected == NULL || actual == NULL ||
        expected->address_result.status != actual->address_result.status ||
        expected->address_commit.target != actual->address_commit.target ||
        expected->address_commit.ar_index != actual->address_commit.ar_index ||
        expected->address_commit.value_after !=
            actual->address_commit.value_after ||
        expected->request.space != actual->request.space ||
        expected->request.access != actual->request.access ||
        expected->request.address_word != actual->request.address_word ||
        expected->request.write_value_word !=
            actual->request.write_value_word) {
        return false;
    }
    expected_effect = &expected->address_result.effect;
    actual_effect = &actual->address_result.effect;
    if (expected_effect->operand.width != actual_effect->operand.width ||
        expected_effect->operand.first_word != actual_effect->operand.first_word ||
        expected_effect->operand.second_word != actual_effect->operand.second_word ||
        expected_effect->source.kind != actual_effect->source.kind ||
        expected_effect->state_policy != actual_effect->state_policy ||
        expected_effect->update.target != actual_effect->update.target ||
        expected_effect->update.phase != actual_effect->update.phase ||
        expected_effect->update.ar_index != actual_effect->update.ar_index ||
        expected_effect->update.value_before !=
            actual_effect->update.value_before ||
        expected_effect->update.value_after != actual_effect->update.value_after) {
        return false;
    }
    if (expected_effect->source.kind == TI_C54X_ADDRESS_SOURCE_AR) {
        return expected_effect->source.value.ar.index ==
                   actual_effect->source.value.ar.index &&
               expected_effect->source.value.ar.value_before ==
                   actual_effect->source.value.ar.value_before;
    }
    if (expected_effect->source.kind ==
        TI_C54X_ADDRESS_SOURCE_AR_INDEXED) {
        return expected_effect->source.value.ar_indexed.index ==
                   actual_effect->source.value.ar_indexed.index &&
               expected_effect->source.value.ar_indexed.value_before ==
                   actual_effect->source.value.ar_indexed.value_before &&
               expected_effect->source.value.ar_indexed.ar0_value_before ==
                   actual_effect->source.value.ar_indexed.ar0_value_before;
    }
    return false;
}

static bool
pc_candidate_matches_live(const ti_c54x_program_pc_bridge_effect *pc,
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
ti_c54x_memory_instruction_candidate_shape_is_valid(
    const struct ti_c54x_memory_instruction_candidate *candidate)
{
    ti_c54x_program_pc_bridge_effect planned = {0};

    if (candidate == NULL ||
        candidate->effect.kind != TI_C54X_OP_EFFECT_MEMORY ||
        !ti_c54x_op_memory_effect_is_valid(&candidate->effect.payload.memory) ||
        !address_resolution_is_valid(&candidate->address_resolution,
                                     &candidate->effect.payload.memory) ||
        candidate->instructions_attempted_after == UINT64_C(0) ||
        candidate->instructions_retired_after == UINT64_C(0)) {
        return false;
    }
    if (ti_c54x_program_pc_bridge_plan(
            candidate->pc_effect.public_pc_word_before,
            candidate->pc_effect.program_word_count, &planned) !=
        TI_C54X_PROGRAM_PC_BRIDGE_OK) {
        return false;
    }
    return planned.public_pc_word_before ==
               candidate->pc_effect.public_pc_word_before &&
           planned.common_pc_word_before ==
               candidate->pc_effect.common_pc_word_before &&
           planned.common_pc_word_after ==
               candidate->pc_effect.common_pc_word_after &&
           planned.public_pc_word_after ==
               candidate->pc_effect.public_pc_word_after &&
           planned.program_word_count == candidate->pc_effect.program_word_count &&
           candidate->address_resolution.request.write_value_word ==
               candidate->effect.payload.memory.write_value_word;
}

bool
ti_c54x_memory_instruction_candidate_matches_live(
    const struct ti_c54x_memory_instruction_candidate *candidate,
    const ti_c54x_arch_state *state)
{
    ti_c54x_memory_address_resolution live = {0};
    const ti_c54x_op_memory_effect *memory;

    if (candidate == NULL || state == NULL) {
        return false;
    }
    memory = &candidate->effect.payload.memory;
    if (ti_c54x_sequencer_resolve_memory_address(
            state, &memory->address_spec, memory->access,
            memory->write_value_word, &live) !=
        TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK ||
        !address_resolution_matches(&candidate->address_resolution, &live) ||
        !pc_candidate_matches_live(&candidate->pc_effect, state)) {
        return false;
    }
    return true;
}
