/*
 * Module contract
 *
 * Owns: root orchestration for one dual-indirect MOD0/MOD1/MOD2/MOD3 WORD16 read
 * pair. It reserves two consecutive IDs, serializes the X then Y endpoint
 * calls, and delegates the READY commit to the dual-commit owner only after
 * both values are ready. The W51B persistent snapshot carries both
 * prevalidated post-access commits; they are validated against the copied
 * mode/selector/address shadows before issue and at every resume boundary.
 * Does not own: opcode semantics, address arithmetic, endpoint storage,
 * timing, CPU lifecycle, observers, diagnostics, allocation, I/O, or global
 * state. No architectural field is written until pair completion passes all
 * candidate and live-state checks.
 */

#include "memory_instruction_dual.h"

#include <stddef.h>
#include <stdint.h>

#include "bus/adapter/word16_adapter.h"
#include "memory_instruction_candidate_validate.h"
#include "memory_instruction_dual_candidate_validate.h"
#include "memory_instruction_dual_commit.h"

static void clear_preserving_sequence(
    ti_c54x_memory_instruction_transaction *transaction)
{
    const uint64_t last_id = transaction->last_transaction_id;

    *transaction = (ti_c54x_memory_instruction_transaction){0};
    transaction->last_transaction_id = last_id;
}

static uint8_t selector_snapshot(uint8_t x_ar, uint8_t y_ar)
{
    /* AR2..AR5 each fit in four bits; keep both in the existing byte slot. */
    return (uint8_t)((x_ar << 4) | (y_ar & UINT8_C(0x0f)));
}

bool ti_c54x_memory_instruction_dual_is_candidate(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    return transaction != NULL && transaction->child_kind ==
                                      TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_WORD16_READ;
}

bool ti_c54x_memory_instruction_dual_prepare_snapshot(
    ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *spec;

    if (transaction == NULL || transaction->candidate.effect.kind !=
                                   TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    spec = &memory->address_spec;
    if (spec->kind != TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16 ||
        memory->access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        memory->complete_dual == NULL || spec->dual.x_mode > UINT8_C(3) ||
        spec->dual.y_mode > UINT8_C(3) || spec->dual.x_ar < UINT8_C(2) ||
        spec->dual.x_ar > UINT8_C(5) || spec->dual.y_ar < UINT8_C(2) ||
        spec->dual.y_ar > UINT8_C(5) || spec->dual.reserved_word0 != 0U ||
        spec->dual.reserved_word1 != 0U || spec->dual.reserved_word2 != 0U ||
        spec->dual.reserved_word3 != 0U ||
        !ti_c54x_memory_instruction_candidate_shape_is_valid(
            &transaction->candidate)) {
        return false;
    }
    transaction->child_kind = TI_C54X_MEMORY_INSTRUCTION_CHILD_DUAL_WORD16_READ;
    transaction->candidate_rmw_operation =
        selector_snapshot(spec->dual.x_ar, spec->dual.y_ar);
    transaction->dual_x_address_word =
        (uint16_t)transaction->candidate.dual_address_snapshot.x_request
            .address_word;
    transaction->dual_y_address_word =
        (uint16_t)transaction->candidate.dual_address_snapshot.y_request
            .address_word;
    return true;
}

static bool candidate_snapshot_matches(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_op_memory_effect *memory;
    const ti_c54x_op_memory_address_spec *spec;

    if (!ti_c54x_memory_instruction_dual_is_candidate(transaction) ||
        transaction->candidate.effect.kind != TI_C54X_OP_EFFECT_MEMORY) {
        return false;
    }
    memory = &transaction->candidate.effect.payload.memory;
    spec = &memory->address_spec;
    return spec->kind == TI_C54X_OP_MEMORY_ADDRESS_DUAL_INDIRECT_WORD16 &&
           spec->dual.x_mode <= UINT8_C(3) &&
           spec->dual.y_mode <= UINT8_C(3) &&
           spec->dual.x_ar >= UINT8_C(2) && spec->dual.x_ar <= UINT8_C(5) &&
           spec->dual.y_ar >= UINT8_C(2) && spec->dual.y_ar <= UINT8_C(5) &&
           spec->dual.reserved_word0 == 0U &&
           spec->dual.reserved_word1 == 0U &&
           spec->dual.reserved_word2 == 0U &&
           spec->dual.reserved_word3 == 0U &&
           memory->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           memory->complete_dual != NULL &&
           transaction->candidate_rmw_operation ==
               selector_snapshot(spec->dual.x_ar, spec->dual.y_ar) &&
           transaction->dual_x_address_word ==
               transaction->candidate.dual_address_snapshot.x_request
                   .address_word &&
           transaction->dual_y_address_word ==
               transaction->candidate.dual_address_snapshot.y_request
                   .address_word &&
           ti_c54x_memory_instruction_dual_snapshot_commits_match_spec(
               &transaction->candidate.dual_address_snapshot, spec);
}

static bool pair_ids_match_root_watermark(
    const ti_c54x_memory_instruction_transaction *transaction)
{
    const ti_c54x_dual_word16_read_request_snapshot *request;

    if (transaction == NULL) {
        return false;
    }
    request = &transaction->dual_bus_transaction.request;
    return request->x_transaction_id != UINT64_C(0) &&
           request->y_transaction_id == request->x_transaction_id +
                                             UINT64_C(1) &&
           request->y_transaction_id != UINT64_C(0) &&
           request->y_transaction_id == transaction->last_transaction_id;
}

typedef enum dual_child_result {
    DUAL_CHILD_READY = 0,
    DUAL_CHILD_WAIT = 1,
    DUAL_CHILD_FAULT = 2,
    DUAL_CHILD_MALFORMED = 3
} dual_child_result;

static dual_child_result advance_child(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, bool begin,
    ti_c54x_bus_status *bus_status)
{
    ti_c54x_dual_word16_read_transaction *pair =
        &transaction->dual_bus_transaction;
    ti_c54x_bus_word16_observation observation = {0};
    ti_c54x_dual_word16_read_issue_result issue;
    ti_c54x_dual_word16_read_status pair_status;
    const ti_c54x_bus_word16_transaction *child = &pair->word16_transaction;

    if (begin) {
        issue = ti_c54x_dual_word16_read_transaction_begin(pair);
        if (issue.status != TI_C54X_DUAL_WORD16_READ_STATUS_OK) {
            return DUAL_CHILD_MALFORMED;
        }
    } else if (child->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
               (pair->state != TI_C54X_DUAL_WORD16_READ_STATE_X &&
                pair->state != TI_C54X_DUAL_WORD16_READ_STATE_Y)) {
        return DUAL_CHILD_MALFORMED;
    }

    endpoint->advance(endpoint->context,
                      begin ? TI_C54X_BUS_WORD16_ADVANCE_BEGIN
                            : TI_C54X_BUS_WORD16_ADVANCE_RESUME,
                      child->transaction_id, &child->request, &observation);
    pair_status = ti_c54x_dual_word16_read_transaction_poll(pair, &observation);
    if (pair_status == TI_C54X_DUAL_WORD16_READ_STATUS_WAIT) {
        return DUAL_CHILD_WAIT;
    }
    if (pair_status == TI_C54X_DUAL_WORD16_READ_STATUS_FAULT) {
        if (bus_status != NULL) {
            *bus_status = pair->staged_bus_status;
        }
        return DUAL_CHILD_FAULT;
    }
    if (pair_status != TI_C54X_DUAL_WORD16_READ_STATUS_OK) {
        return DUAL_CHILD_MALFORMED;
    }
    return DUAL_CHILD_READY;
}

static bool prepare_pair(ti_c54x_memory_instruction_transaction *transaction)
{
    ti_c54x_dual_word16_read_request request = {0};
    const ti_c54x_dual_memory_address_snapshot *snapshot =
        &transaction->candidate.dual_address_snapshot;

    request.x_request = snapshot->x_request;
    request.y_request = snapshot->y_request;
    request.x_transaction_id = transaction->last_transaction_id - UINT64_C(1);
    request.y_transaction_id = transaction->last_transaction_id;
    return ti_c54x_dual_word16_read_transaction_prepare(
               &transaction->dual_bus_transaction, &request) ==
           TI_C54X_DUAL_WORD16_READ_STATUS_OK;
}

static ti_c54x_memory_instruction_status reject_staged(
    ti_c54x_memory_instruction_transaction *transaction,
    ti_c54x_memory_instruction_result *result)
{
    /* The borrowed root watermark may have been edited while the endpoint
     * callback was running. Restore the frozen Y ID before abandoning staged
     * work so a later reset cannot reuse or skip a pair. */
    if (ti_c54x_dual_word16_read_transaction_is_canonical(
            &transaction->dual_bus_transaction) &&
        transaction->dual_bus_transaction.request.y_transaction_id !=
            UINT64_C(0)) {
        transaction->last_transaction_id =
            transaction->dual_bus_transaction.request.y_transaction_id;
    }
    (void)ti_c54x_dual_word16_read_transaction_abandon_staged(
        &transaction->dual_bus_transaction);
    transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
    result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
}

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_dual_advance(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint, ti_c54x_arch_state *state,
    ti_c54x_accumulator_state *accumulators,
    ti_c54x_t_register_state *t_register,
    ti_c54x_memory_instruction_result *result)
{
    const bool first_issue = transaction != NULL &&
                             transaction->phase ==
                                 TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED;
    ti_c54x_bus_status bus_status = TI_C54X_BUS_OK;
    dual_child_result child_result;

    if (result != NULL) {
        *result = (ti_c54x_memory_instruction_result){0};
    }
    if (transaction == NULL || endpoint == NULL || state == NULL ||
        accumulators == NULL || t_register == NULL || result == NULL ||
        !ti_c54x_memory_instruction_dual_is_candidate(transaction) ||
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
        if (transaction->last_transaction_id > UINT64_MAX - UINT64_C(2) ||
            transaction->dual_bus_transaction.state !=
                TI_C54X_DUAL_WORD16_READ_STATE_IDLE) {
            result->status =
                transaction->last_transaction_id > UINT64_MAX - UINT64_C(2)
                    ? TI_C54X_MEMORY_INSTRUCTION_STATUS_ID_EXHAUSTED
                    : TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        transaction->last_transaction_id += UINT64_C(2);
        if (!prepare_pair(transaction)) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
            return result->status;
        }
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING;
    } else if (!ti_c54x_dual_word16_read_transaction_is_canonical(
                   &transaction->dual_bus_transaction) ||
               !pair_ids_match_root_watermark(transaction)) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_CANDIDATE;
    }

    child_result = advance_child(transaction, endpoint, first_issue,
                                 &bus_status);
    result->transaction_id =
        transaction->dual_bus_transaction.word16_transaction.transaction_id;
    result->instructions_attempted = first_issue ? UINT64_C(1) : UINT64_C(0);
    if (child_result == DUAL_CHILD_WAIT) {
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (child_result == DUAL_CHILD_MALFORMED) {
        result->status =
            TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
    }
    if (child_result == DUAL_CHILD_FAULT) {
        transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
        result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
        result->bus_status = bus_status;
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }

    /* X READY has only staged the first value and prepared Y. */
    if (transaction->dual_bus_transaction.state ==
        TI_C54X_DUAL_WORD16_READ_STATE_Y) {
        if (!candidate_snapshot_matches(transaction) ||
            !pair_ids_match_root_watermark(transaction) ||
            !ti_c54x_memory_instruction_candidate_matches_live(
                &transaction->candidate, state)) {
            return reject_staged(transaction, result);
        }
        child_result = advance_child(transaction, endpoint, true, &bus_status);
        result->transaction_id = transaction->dual_bus_transaction
                                     .word16_transaction.transaction_id;
        if (child_result == DUAL_CHILD_WAIT) {
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_WAIT;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
        if (child_result == DUAL_CHILD_MALFORMED) {
            result->status =
                TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
        }
        if (child_result == DUAL_CHILD_FAULT) {
            transaction->phase = TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT;
            result->status = TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT;
            result->bus_status = bus_status;
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
        }
    }

    if (!candidate_snapshot_matches(transaction) ||
        !pair_ids_match_root_watermark(transaction) ||
        !ti_c54x_memory_instruction_candidate_matches_live(
            &transaction->candidate, state)) {
        return reject_staged(transaction, result);
    }
    {
        ti_c54x_dual_word16_read_completion_result completion =
            ti_c54x_dual_word16_read_transaction_complete(
                &transaction->dual_bus_transaction);
        ti_c54x_memory_instruction_status status;

        if (completion.status != TI_C54X_DUAL_WORD16_READ_STATUS_OK) {
            result->status =
                completion.status == TI_C54X_DUAL_WORD16_READ_STATUS_FAULT
                    ? TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT
                    : TI_C54X_MEMORY_INSTRUCTION_STATUS_MALFORMED_OBSERVATION;
            result->bus_status = completion.bus_status;
            return result->status == TI_C54X_MEMORY_INSTRUCTION_STATUS_FAULT
                       ? TI_C54X_MEMORY_INSTRUCTION_STATUS_OK
                       : result->status;
        }
        status = ti_c54x_memory_instruction_dual_commit_ready(
            transaction, state, accumulators, t_register,
            completion.x_value_word, completion.y_value_word);
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

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_dual_cancel(
    ti_c54x_memory_instruction_transaction *transaction,
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    ti_c54x_dual_word16_read_transaction *pair;
    const ti_c54x_bus_word16_transaction *child;

    if (!ti_c54x_memory_instruction_dual_is_candidate(transaction)) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        if (transaction->dual_bus_transaction.state !=
            TI_C54X_DUAL_WORD16_READ_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
        clear_preserving_sequence(transaction);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    pair = &transaction->dual_bus_transaction;
    child = &pair->word16_transaction;
    if ((pair->state != TI_C54X_DUAL_WORD16_READ_STATE_X &&
         pair->state != TI_C54X_DUAL_WORD16_READ_STATE_Y) ||
        !ti_c54x_dual_word16_read_transaction_is_canonical(pair) ||
        child->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
        !pair_ids_match_root_watermark(transaction) ||
        ti_c54x_bus_word16_adapter_validate_endpoint(endpoint) !=
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (endpoint->cancel(endpoint->context, child->transaction_id,
                         &child->request) !=
        TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED;
    }
    if (ti_c54x_dual_word16_read_transaction_cancel(pair,
                                                     child->transaction_id) !=
        TI_C54X_DUAL_WORD16_READ_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCEL_FAILED;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_CANCELLED;
}

ti_c54x_memory_instruction_status ti_c54x_memory_instruction_dual_reset(
    ti_c54x_memory_instruction_transaction *transaction)
{
    if (!ti_c54x_memory_instruction_dual_is_candidate(transaction) ||
        transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PENDING) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->phase == TI_C54X_MEMORY_INSTRUCTION_PHASE_PREPARED) {
        if (transaction->dual_bus_transaction.state !=
            TI_C54X_DUAL_WORD16_READ_STATE_IDLE) {
            return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
        }
        clear_preserving_sequence(transaction);
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
    }
    if (transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_COMPLETE &&
        transaction->phase != TI_C54X_MEMORY_INSTRUCTION_PHASE_FAULT) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    if (ti_c54x_dual_word16_read_transaction_reset(
            &transaction->dual_bus_transaction) !=
        TI_C54X_DUAL_WORD16_READ_STATUS_OK) {
        return TI_C54X_MEMORY_INSTRUCTION_STATUS_INVALID_ARGUMENT;
    }
    clear_preserving_sequence(transaction);
    return TI_C54X_MEMORY_INSTRUCTION_STATUS_OK;
}
