/*
 * Module contract
 *
 * Owns: the root LONG32 memory-instruction seam, including deferred pair
 * preparation, two-ID reservation, direct endpoint orchestration, copied
 * MSW/LSW completion, and terminal cancellation/reset.
 * Does not own: opcode decode, address arithmetic, CPU lifecycle, endpoint
 * storage, timing, observers, diagnostics, or global state.
 */

#include "memory_instruction_long32.h"

#include <stddef.h>

#include "bus/adapter/word16_adapter.h"
#include "long32_pair_transaction_validate.h"
#include "sequencer/effect_transaction.h"
#include "memory_instruction_candidate_validate.h"

static void clear_preserving_sequence(
    ti_c54x_memory_instruction_transaction *transaction)
{
    const uint64_t last_id = transaction->last_transaction_id;

    *transaction = (ti_c54x_memory_instruction_transaction){0};
    transaction->last_transaction_id = last_id;
}

bool ti_c54x_memory_instruction_long32_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    return transaction != NULL && transaction->child_kind ==
                                      TI_C54X_MEMORY_INSTRUCTION_CHILD_LONG32;
}

bool ti_c54x_memory_instruction_long32_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;

    if (transaction == NULL || transaction->candidate.effect.kind !=
                                   TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    if (memory->address_spec.kind !=
        TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32) {
        return false;
    }
    transaction->child_kind = TI_C54X_MEMORY_INSTRUCTION_CHILD_LONG32;
    transaction->long32_first_value_msw = memory->first_value_msw;
    transaction->long32_second_value_lsw = memory->second_value_lsw;
    transaction->candidate_rmw_operation = UINT8_C(0);
    return true;
}

static bool candidate_snapshot_matches(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;

    if (!ti_c54x_memory_instruction_long32_is_candidate(transaction) ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    return memory->address_spec.kind ==
               TI_C54X_OP_MEMORY_ADDRESS_LONG_OFFSET_MOD13_LONG32 &&
           memory->first_value_msw == transaction->long32_first_value_msw &&
           memory->second_value_lsw ==
               transaction->long32_second_value_lsw;
}

static bool pair_ids_match_root_watermark(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_long32_pair_request *request;

    if (transaction == NULL) {
        return false;
    }
    request = &transaction->long32_bus_transaction.request;
    return request->first_transaction_id != UINT64_MAX &&
           request->second_transaction_id ==
               request->first_transaction_id + UINT64_C(1) &&
           request->second_transaction_id == transaction->last_transaction_id;
}

static ti_c54x_memory_instruction_status commit_architecture(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state)
{
    const ti_c54x_sequencer_address_commit *commit =
        &transaction->candidate.address_resolution.address_commit;

    if (commit->target == TI_C54X_ADDRESS_UPDATE_AR) {
        state->ar[commit->ar_index] = commit->value_after;
        state->ar_known_mask |= (uint8_t)(UINT8_C(1) << commit->ar_index);
    }
    state->pc_word = transaction->candidate.pc_effect.public_pc_word_after;
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
}

static ti_c54x_memory_instruction_status commit_ready(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_arch_state *state, ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register, uint16_t first_msw,
    uint16_t second_lsw)
{
    const ti_c54x_op_memory_effect *memory =
        &transaction->candidate.effect.payload.memory;
    ti_c54x_op_effect next_effect = {0};
    ti_c54x_effect_transaction ordinary = {0};

    if (memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        return commit_architecture(transaction, state);
    }
    if (memory->complete_long32 == NULL ||
        memory->complete_long32(&transaction->candidate.instruction,
                                &transaction->candidate.input, first_msw,
                                second_lsw, &next_effect) !=
            TI_C54X_OP_STATUS_OK ||
        ti_c54x_effect_transaction_stage(
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

typedef enum long32_child_result {
    LONG32_CHILD_READY = 0,
    LONG32_CHILD_WAIT = 1,
    LONG32_CHILD_FAULT = 2,
    LONG32_CHILD_MALFORMED = 3
} long32_child_result;

static long32_child_result advance_child(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, bool begin,
    ti_c54x_bus_status *bus_status)
{
    ti_c54x_long32_pair_transaction *pair =
        &transaction->long32_bus_transaction;
    ti_c54x_bus_word16_observation observation = {0};
    ti_c54x_long32_pair_issue_result issue;
    ti_c54x_long32_pair_status pair_status;
    const ti_c54x_bus_word16_transaction *child = &pair->word16_transaction;

    if (begin) {
        issue = ti_c54x_long32_pair_transaction_begin(pair);
        if (issue.status != TI_C54X_LONG32_PAIR_STATUS_OK) {
            return LONG32_CHILD_MALFORMED;
        }
    } else if (child->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
        return LONG32_CHILD_MALFORMED;
    }

    endpoint->advance(endpoint->context,
                      begin ? TI_C54X_BUS_WORD16_ADVANCE_BEGIN
                            : TI_C54X_BUS_WORD16_ADVANCE_RESUME,
                      child->transaction_id, &child->request, &observation);
    pair_status = ti_c54x_long32_pair_transaction_poll(pair, &observation);
    if (pair_status == TI_C54X_LONG32_PAIR_STATUS_WAIT) {
        return LONG32_CHILD_WAIT;
    }
    if (pair_status == TI_C54X_LONG32_PAIR_STATUS_FAULT) {
        if (bus_status != NULL) {
            *bus_status = pair->staged_bus_status;
        }
        return LONG32_CHILD_FAULT;
    }
    if (pair_status != TI_C54X_LONG32_PAIR_STATUS_OK) {
        return LONG32_CHILD_MALFORMED;
    }
    return LONG32_CHILD_READY;
}

static bool prepare_pair(
    ti_c54x_memory_instruction_transaction *transaction)
{
    ti_c54x_long32_pair_request request = {0};
    const ti_c54x_op_memory_effect *memory =
        &transaction->candidate.effect.payload.memory;
    const ti_c54x_data_operand *operand =
        &transaction->candidate.address_resolution.address_result.effect.operand;

    request.operand = *operand;
    request.access = memory->access;
    request.first_value_msw = memory->first_value_msw;
    request.second_value_lsw = memory->second_value_lsw;
    request.first_transaction_id = transaction->last_transaction_id - UINT64_C(1);
    request.second_transaction_id = transaction->last_transaction_id;
    return ti_c54x_long32_pair_transaction_prepare(
               &transaction->long32_bus_transaction, &request) ==
           TI_C54X_LONG32_PAIR_STATUS_OK;
}

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_long32_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result)
{
    const bool first_issue = transaction->phase ==
                             TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    long32_child_result child_result;
    ti_c54x_bus_status bus_status = TI_C54X_BUS_OK;

    if (!ti_c54x_memory_instruction_long32_is_candidate(transaction) ||
        (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED &&
         transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) ||
        !ti_c54x_memory_instruction_candidate_shape_is_valid(
            &transaction->candidate) ||
        !candidate_snapshot_matches(transaction) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        if (result != NULL) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
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
        if (transaction->last_transaction_id > UINT64_MAX - UINT64_C(2)) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED;
        }
        transaction->last_transaction_id += UINT64_C(2);
        if (!prepare_pair(transaction)) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
    } else if (!ti_c54x_long32_pair_transaction_is_canonical(
                   &transaction->long32_bus_transaction) ||
               !pair_ids_match_root_watermark(transaction)) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }
    child_result = advance_child(transaction, endpoint, first_issue,
                                 &bus_status);
    result->transaction_id =
        transaction->long32_bus_transaction.word16_transaction.transaction_id;
    result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
    if (child_result == LONG32_CHILD_WAIT) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (child_result == LONG32_CHILD_MALFORMED) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (child_result == LONG32_CHILD_FAULT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
        result->bus_status = bus_status;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (transaction->long32_bus_transaction.state ==
        TI_C54X_LONG32_PAIR_STATE_SECOND) {
        const bool ids_match = pair_ids_match_root_watermark(transaction);

        if (!candidate_snapshot_matches(transaction) || !ids_match ||
            !ti_c54x_memory_instruction_candidate_matches_live(
                &transaction->candidate, state)) {
            if (!ids_match &&
                ti_c54x_long32_pair_transaction_is_canonical(
                    &transaction->long32_bus_transaction)) {
                transaction->last_transaction_id =
                    transaction->long32_bus_transaction.request
                        .second_transaction_id;
            }
            (void)ti_c54x_long32_pair_transaction_abandon_staged(
                &transaction->long32_bus_transaction);
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
        child_result = advance_child(transaction, endpoint, true, &bus_status);
        result->transaction_id = transaction->long32_bus_transaction
                                     .word16_transaction.transaction_id;
        if (child_result == LONG32_CHILD_WAIT) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (child_result == LONG32_CHILD_MALFORMED) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (child_result == LONG32_CHILD_FAULT) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
            result->bus_status = bus_status;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
    }
    {
        const bool ids_match = pair_ids_match_root_watermark(transaction);
        if (!candidate_snapshot_matches(transaction) || !ids_match ||
            !ti_c54x_memory_instruction_candidate_matches_live(
                &transaction->candidate, state)) {
            /* A host can mutate the root watermark through the borrowed
             * transaction object.  Restore the frozen pair watermark before
             * clearing staged work so a reset cannot reuse or skip IDs. */
            if (!ids_match &&
                ti_c54x_long32_pair_transaction_is_canonical(
                    &transaction->long32_bus_transaction)) {
                transaction->last_transaction_id =
                    transaction->long32_bus_transaction.request
                        .second_transaction_id;
            }
            (void)ti_c54x_long32_pair_transaction_abandon_staged(
                &transaction->long32_bus_transaction);
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        }
    }
    {
        ti_c54x_long32_pair_completion_result completion =
            ti_c54x_long32_pair_transaction_complete(
                &transaction->long32_bus_transaction);
        ti_c54x_memory_instruction_status status;

        if (completion.status != TI_C54X_LONG32_PAIR_STATUS_OK) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        }
        status = commit_ready(transaction, state, accumulators, t_register,
                              completion.first_msw, completion.second_lsw);
        if (status != TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED) {
            result->status = status;
            return status;
        }
    }
    result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_RETIRED;
    result->bus_status = TI_C54X_BUS_OK;
    result->instructions_retired = UINT64_C(1);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_long32_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    ti_c54x_long32_pair_transaction *pair;
    const ti_c54x_bus_word16_transaction *child;

    if (!ti_c54x_memory_instruction_long32_is_candidate(transaction)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        clear_preserving_sequence(transaction);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    pair = &transaction->long32_bus_transaction;
    child = &pair->word16_transaction;
    if (child->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
        ti_c54x_bus_word16_adapter_validate_endpoint(endpoint) !=
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (endpoint->cancel(endpoint->context, child->transaction_id,
                         &child->request) !=
        TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED;
    }
    if (ti_c54x_long32_pair_transaction_cancel(pair, child->transaction_id) !=
        TI_C54X_LONG32_PAIR_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
}

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_long32_reset(
    ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_long32_is_candidate(transaction) ||
        transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        clear_preserving_sequence(transaction);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE &&
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (ti_c54x_long32_pair_transaction_reset(
            &transaction->long32_bus_transaction) !=
        TI_C54X_LONG32_PAIR_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}
