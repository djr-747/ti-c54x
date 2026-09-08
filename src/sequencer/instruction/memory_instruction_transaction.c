/*
 * Module contract
 *
 * Owns: the allocation-free root transaction that composes memory address
 * resolution, the WORD16 reducer, static opcode completion, ordinary effects,
 * and sequential PC/counter candidates.
 * Does not own: CPU lifecycle, physical memory, endpoint context, opcode
 * dispatch, timing, observers, diagnostics, or global state.
 */

#include "memory_instruction_transaction.h"

#include <stdbool.h>
#include <stddef.h>

#include "bus/adapter/word16_adapter.h"
#include "sequencer/effect_transaction.h"

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
        resolution->address_result.effect.source.kind !=
            TI_C54X_ADDRESS_SOURCE_AR ||
        resolution->address_result.effect.source.value.ar.index !=
            effect->address_spec.arf ||
        resolution->request.space != TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        resolution->request.access != effect->access ||
        resolution->request.address_word !=
            resolution->address_result.effect.operand.first_word ||
        !ti_c54x_bus_word16_request_is_valid(&resolution->request)) {
        return false;
    }
    address_effect = &resolution->address_result.effect;
    address_commit = &resolution->address_commit;

    /* The resolved address effect does not retain its originating mode
     * discriminator, so recheck the mode-specific operand/update contract
     * against the memory effect. This keeps forged candidates from reaching
     * the endpoint while leaving arithmetic to the address planner. */
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
        return effect->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
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

static bool
candidate_shape_is_valid(const ti_c54x_memory_instruction_candidate *candidate)
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

static bool
candidate_matches_live(const ti_c54x_memory_instruction_candidate *candidate,
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

static void
clear_preserving_sequence(ti_c54x_memory_instruction_transaction *transaction)
{
    const uint64_t last_id = transaction->last_transaction_id;

    *transaction = (ti_c54x_memory_instruction_transaction){0};
    transaction->last_transaction_id = last_id;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_prepare(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_decoded_instruction *instruction,
    const ti_c54x_op_input_snapshot *input,
    const ti_c54x_op_effect *effect,
    const ti_c54x_arch_state *state,
    uint64_t instructions_attempted_before,
    uint64_t instructions_retired_before)
{
    ti_c54x_memory_instruction_candidate candidate = {0};
    ti_c54x_memory_address_resolution resolution = {0};
    ti_c54x_program_pc_bridge_effect pc_effect = {0};
    ti_c54x_memory_instruction_transaction *target = transaction;

    if (target == NULL || instruction == NULL || input == NULL ||
        effect == NULL || state == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (target->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE ||
        target->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (effect->kind != TI_C54X_OP_EFFECT_MEMORY ||
        !ti_c54x_op_memory_effect_is_valid(&effect->payload.memory)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (instructions_attempted_before == UINT64_MAX ||
        instructions_retired_before == UINT64_MAX) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_COUNTER_OVERFLOW;
    }
    if (ti_c54x_sequencer_resolve_memory_address(
            state, &effect->payload.memory.address_spec,
            effect->payload.memory.access,
            effect->payload.memory.write_value_word, &resolution) !=
        TI_C54X_MEMORY_ADDRESS_RESOLUTION_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (ti_c54x_program_pc_bridge_plan(
            state->pc_word, instruction->bundle.program_word_count,
            &pc_effect) != TI_C54X_PROGRAM_PC_BRIDGE_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    candidate.instruction = *instruction;
    candidate.input = *input;
    candidate.effect = *effect;
    candidate.address_resolution = resolution;
    candidate.pc_effect = pc_effect;
    candidate.instructions_attempted_after =
        instructions_attempted_before + UINT64_C(1);
    candidate.instructions_retired_after = instructions_retired_before +
                                           UINT64_C(1);
    target->candidate = candidate;
    target->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

static ti_c54x_memory_instruction_status
commit_architecture(ti_c54x_memory_instruction_transaction *transaction,
                    ti_c54x_arch_state *state)
{
    const ti_c54x_sequencer_address_commit *const address_commit =
        &transaction->candidate.address_resolution.address_commit;

    /* Candidate and live-state validation admitted only NONE or a valid AR
     * target before endpoint issue. Keep READY publication infallible. */
    if (address_commit->target == TI_C54X_ADDRESS_UPDATE_AR) {
        state->ar[address_commit->ar_index] = address_commit->value_after;
        state->ar_known_mask |=
            (uint8_t)(UINT8_C(1) << address_commit->ar_index);
    }
    state->pc_word = transaction->candidate.pc_effect.public_pc_word_after;
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
}

static ti_c54x_memory_instruction_status
commit_ready(ti_c54x_memory_instruction_transaction *transaction,
             ti_c54x_arch_state *state,
             ti_c54x_accumulator_state *accumulators,
             ti_c54x_t_register_state *t_register,
             uint16_t read_value_word)
{
    ti_c54x_effect_transaction ordinary = {0};
    ti_c54x_op_effect next_effect = {0};
    const ti_c54x_op_memory_effect *memory =
        &transaction->candidate.effect.payload.memory;

    /* A WRITE READY has already published externally; only infallible scalar
     * assignments remain. No validator, callback, or effect API is called. */
    if (memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return commit_architecture(transaction, state);
    }
    if (memory->complete == NULL || memory->complete(
                         &transaction->candidate.instruction,
                         &transaction->candidate.input, read_value_word,
                         &next_effect) != TI_C54X_OP_STATUS_OK) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    if (ti_c54x_effect_transaction_stage(
            &next_effect, &state->status, accumulators, t_register,
            &ordinary) != TI_C54X_EFFECT_TRANSACTION_OK ||
        ti_c54x_effect_transaction_commit(
            &ordinary, &state->status, accumulators, t_register) !=
            TI_C54X_EFFECT_TRANSACTION_OK) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    return commit_architecture(transaction, state);
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result)
{
    ti_c54x_bus_word16_adapter_result adapter_result;
    ti_c54x_memory_instruction_status operation_status;
    bool first_issue;

    if (result != NULL) {
        *result = (ti_c54x_memory_instruction_result){0};
    }
    if (transaction == NULL || endpoint == NULL || state == NULL ||
        accumulators == NULL || t_register == NULL || result == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED &&
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    first_issue = transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    if (first_issue) {
        if (transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE ||
            !candidate_shape_is_valid(&transaction->candidate)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    } else if (transaction->bus_transaction.state !=
                   TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
               transaction->last_transaction_id == UINT64_C(0) ||
               transaction->bus_transaction.transaction_id !=
                   transaction->last_transaction_id) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    {
        const ti_c54x_bus_word16_adapter_status endpoint_status =
            ti_c54x_bus_word16_adapter_validate_endpoint(endpoint);
        if (endpoint_status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_NO_CALLBACK) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
            result->bus_status = TI_C54X_BUS_NO_CALLBACK;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (endpoint_status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    }

    if (first_issue) {
        uint64_t next_id;

        if (!candidate_matches_live(&transaction->candidate, state)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }

        if (transaction->last_transaction_id == UINT64_MAX) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
        }
        next_id = transaction->last_transaction_id + UINT64_C(1);
        if (next_id == UINT64_C(0)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
        }
        if (ti_c54x_bus_word16_transaction_prepare(
                &transaction->bus_transaction,
                &transaction->candidate.address_resolution.request,
                next_id) != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        transaction->last_transaction_id = next_id;
        result->transaction_id = next_id;
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
    } else {
        result->transaction_id = transaction->last_transaction_id;
    }

    adapter_result = ti_c54x_bus_word16_adapter_advance(
        endpoint,
        first_issue ? TI_C54X_BUS_WORD16_ADVANCE_BEGIN
                    : TI_C54X_BUS_WORD16_ADVANCE_RESUME,
        &transaction->bus_transaction);
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
        result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (adapter_result.status ==
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_OBSERVATION) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
    }
    if (adapter_result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
        result->bus_status = adapter_result.bus_status;
        result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
    }
    operation_status = commit_ready(transaction, state, accumulators,
                                     t_register, adapter_result.read_value_word);
    if (operation_status != TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
        return operation_status;
    }
    result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
    result->bus_status = TI_C54X_BUS_OK;
    result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
    result->instructions_retired = UINT64_C(1);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    if (transaction == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        if (transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
        clear_preserving_sequence(transaction);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING ||
        transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
        transaction->last_transaction_id == UINT64_C(0) ||
        transaction->bus_transaction.transaction_id !=
            transaction->last_transaction_id) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    {
        const ti_c54x_bus_word16_adapter_result adapter_result =
            ti_c54x_bus_word16_adapter_cancel(
                endpoint, &transaction->bus_transaction);
        if (adapter_result.status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED;
        }
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
}

ti_c54x_memory_instruction_status
ti_c54x_memory_instruction_transaction_reset(
    ti_c54x_memory_instruction_transaction *transaction)
{
    if (transaction == NULL) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_IDLE ||
        transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        if (transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    } else if (transaction->phase ==
                   TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE) {
        if (transaction->bus_transaction.state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    } else if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT) {
        if (transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT &&
            transaction->bus_transaction.state !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
    } else {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (ti_c54x_bus_word16_transaction_reset(&transaction->bus_transaction) !=
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}
