/*
 * Module contract
 *
 * Owns: the allocation-free state reducer for one copied LONG32 data
 * operand, including deterministic MSW/LSW WORD16 request order and staged
 * pair publication.
 * Does not own: endpoint callbacks, physical memory or visibility, bus
 * arbitration, timing, CPU state, opcode or effect publication, observers,
 * diagnostics, environment, or global state. The child transaction is the
 * only active request storage and is reused once for the second word.
 */

#include "long32_pair_transaction.h"

#include <stddef.h>

#include "long32_pair_transaction_validate.h"

static ti_c54x_long32_pair_status ti_c54x_long32_pair_map_status(
    ti_c54x_bus_word16_transaction_status status)
{
    switch (status) {
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK:
        return TI_C54X_LONG32_PAIR_STATUS_OK;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT:
        return TI_C54X_LONG32_PAIR_STATUS_WAIT;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT:
        return TI_C54X_LONG32_PAIR_STATUS_FAULT;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT:
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE:
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION:
        return TI_C54X_LONG32_PAIR_STATUS_STALE_TRANSACTION;
    default:
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
}

static ti_c54x_long32_pair_issue_result
ti_c54x_long32_pair_invalid_issue(ti_c54x_long32_pair_status status)
{
    ti_c54x_long32_pair_issue_result result = {0};

    result.status = status;
    return result;
}

static ti_c54x_long32_pair_completion_result
ti_c54x_long32_pair_invalid_completion(ti_c54x_long32_pair_status status)
{
    ti_c54x_long32_pair_completion_result result = {0};

    result.status = status;
    return result;
}

static void ti_c54x_long32_pair_clear(
    ti_c54x_long32_pair_transaction *transaction)
{
    *transaction = (ti_c54x_long32_pair_transaction){0};
    transaction->staged_bus_status = TI_C54X_BUS_OK;
}

ti_c54x_long32_pair_status ti_c54x_long32_pair_transaction_prepare(
    ti_c54x_long32_pair_transaction *transaction,
    const ti_c54x_long32_pair_request *request)
{
    ti_c54x_bus_word16_request first_request;

    if (transaction == NULL || request == NULL) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_long32_pair_transaction_is_canonical(transaction) ||
        transaction->state != TI_C54X_LONG32_PAIR_STATE_IDLE) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
    if (!ti_c54x_long32_pair_request_is_valid(request) ||
        !ti_c54x_long32_pair_build_word16_request(
            request, false, &first_request)) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT;
    }
    transaction->request = *request;
    if (ti_c54x_bus_word16_transaction_prepare(
            &transaction->word16_transaction, &first_request,
            request->first_transaction_id) !=
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        transaction->request = (ti_c54x_long32_pair_request){0};
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
    transaction->staged_first_msw = UINT16_C(0);
    transaction->staged_second_lsw = UINT16_C(0);
    transaction->staged_bus_status = TI_C54X_BUS_OK;
    transaction->state = TI_C54X_LONG32_PAIR_STATE_FIRST;
    return TI_C54X_LONG32_PAIR_STATUS_OK;
}

ti_c54x_long32_pair_issue_result ti_c54x_long32_pair_transaction_begin(
    ti_c54x_long32_pair_transaction *transaction)
{
    ti_c54x_bus_word16_transaction_issue_result child;
    ti_c54x_long32_pair_issue_result result;

    if (transaction == NULL) {
        return ti_c54x_long32_pair_invalid_issue(
            TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
    }
    if (!ti_c54x_long32_pair_transaction_is_canonical(transaction)) {
        return ti_c54x_long32_pair_invalid_issue(
            TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
    }
    if (transaction->state != TI_C54X_LONG32_PAIR_STATE_FIRST &&
        transaction->state != TI_C54X_LONG32_PAIR_STATE_SECOND) {
        return ti_c54x_long32_pair_invalid_issue(
            TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
    }
    child = ti_c54x_bus_word16_transaction_begin(
        &transaction->word16_transaction);
    if (child.status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return ti_c54x_long32_pair_invalid_issue(
            ti_c54x_long32_pair_map_status(child.status));
    }
    result.status = TI_C54X_LONG32_PAIR_STATUS_OK;
    result.transaction_id = child.transaction_id;
    result.request = child.request;
    return result;
}

static ti_c54x_long32_pair_status ti_c54x_long32_pair_finish_child(
    ti_c54x_long32_pair_transaction *transaction,
    bool second)
{
    ti_c54x_bus_word16_transaction_completion_result completion;

    completion = ti_c54x_bus_word16_transaction_complete(
        &transaction->word16_transaction,
        transaction->word16_transaction.transaction_id);
    if (completion.status == TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT) {
        transaction->staged_first_msw = UINT16_C(0);
        transaction->staged_second_lsw = UINT16_C(0);
        transaction->staged_bus_status = completion.bus_status;
        transaction->state = TI_C54X_LONG32_PAIR_STATE_FAULT;
        return TI_C54X_LONG32_PAIR_STATUS_FAULT;
    }
    if (completion.status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return ti_c54x_long32_pair_map_status(completion.status);
    }
    if (second) {
        if (transaction->request.access ==
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
            transaction->staged_second_lsw = completion.read_value_word;
        } else {
            transaction->staged_second_lsw = UINT16_C(0);
        }
        transaction->staged_bus_status = completion.bus_status;
        transaction->state = TI_C54X_LONG32_PAIR_STATE_COMPLETION_READY;
        return TI_C54X_LONG32_PAIR_STATUS_OK;
    }
    if (transaction->request.access ==
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        transaction->staged_first_msw = completion.read_value_word;
    } else {
        transaction->staged_first_msw = UINT16_C(0);
    }
    return TI_C54X_LONG32_PAIR_STATUS_OK;
}

ti_c54x_long32_pair_status ti_c54x_long32_pair_transaction_poll(
    ti_c54x_long32_pair_transaction *transaction,
    const ti_c54x_bus_word16_observation *observation)
{
    ti_c54x_bus_word16_request second_request;
    ti_c54x_bus_word16_transaction_status child_status;
    const bool second = transaction != NULL &&
                        transaction->state ==
                            TI_C54X_LONG32_PAIR_STATE_SECOND;
    ti_c54x_long32_pair_status status;

    if (transaction == NULL || observation == NULL) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_long32_pair_transaction_is_canonical(transaction)) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
    if (transaction->state != TI_C54X_LONG32_PAIR_STATE_FIRST &&
        transaction->state != TI_C54X_LONG32_PAIR_STATE_SECOND) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
    child_status = ti_c54x_bus_word16_transaction_poll(
        &transaction->word16_transaction, observation);
    status = ti_c54x_long32_pair_map_status(child_status);
    if (status != TI_C54X_LONG32_PAIR_STATUS_OK &&
        status != TI_C54X_LONG32_PAIR_STATUS_FAULT) {
        return status;
    }
    status = ti_c54x_long32_pair_finish_child(transaction, second);
    if (status == TI_C54X_LONG32_PAIR_STATUS_FAULT) {
        return status;
    }
    if (status != TI_C54X_LONG32_PAIR_STATUS_OK) {
        return status;
    }
    if (second) {
        return TI_C54X_LONG32_PAIR_STATUS_OK;
    }
    if (!ti_c54x_long32_pair_build_word16_request(
            &transaction->request, true, &second_request)) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
    if (ti_c54x_bus_word16_transaction_reset(
            &transaction->word16_transaction) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK ||
        ti_c54x_bus_word16_transaction_prepare(
            &transaction->word16_transaction, &second_request,
            transaction->request.second_transaction_id) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
    transaction->state = TI_C54X_LONG32_PAIR_STATE_SECOND;
    return TI_C54X_LONG32_PAIR_STATUS_OK;
}

ti_c54x_long32_pair_status ti_c54x_long32_pair_transaction_cancel(
    ti_c54x_long32_pair_transaction *transaction,
    uint64_t transaction_id)
{
    ti_c54x_bus_word16_transaction_status status;

    if (transaction == NULL) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_long32_pair_transaction_is_canonical(transaction)) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
    if (transaction->state != TI_C54X_LONG32_PAIR_STATE_FIRST &&
        transaction->state != TI_C54X_LONG32_PAIR_STATE_SECOND) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
    if (transaction->word16_transaction.state !=
        TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
    if (transaction_id == UINT64_C(0)) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT;
    }
    if (transaction_id != transaction->word16_transaction.transaction_id) {
        return TI_C54X_LONG32_PAIR_STATUS_STALE_TRANSACTION;
    }
    status = ti_c54x_bus_word16_transaction_cancel(
        &transaction->word16_transaction, transaction_id);
    if (status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return ti_c54x_long32_pair_map_status(status);
    }
    ti_c54x_long32_pair_clear(transaction);
    return TI_C54X_LONG32_PAIR_STATUS_OK;
}

ti_c54x_long32_pair_completion_result
ti_c54x_long32_pair_transaction_complete(
    ti_c54x_long32_pair_transaction *transaction)
{
    ti_c54x_long32_pair_completion_result result;

    if (transaction == NULL) {
        return ti_c54x_long32_pair_invalid_completion(
            TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
    }
    if (!ti_c54x_long32_pair_transaction_is_canonical(transaction)) {
        return ti_c54x_long32_pair_invalid_completion(
            TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
    }
    if (transaction->state == TI_C54X_LONG32_PAIR_STATE_FAULT) {
        result = ti_c54x_long32_pair_invalid_completion(
            TI_C54X_LONG32_PAIR_STATUS_FAULT);
        result.bus_status = transaction->staged_bus_status;
        return result;
    }
    if (transaction->state == TI_C54X_LONG32_PAIR_STATE_FIRST ||
        transaction->state == TI_C54X_LONG32_PAIR_STATE_SECOND) {
        if (transaction->word16_transaction.state ==
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
            result = ti_c54x_long32_pair_invalid_completion(
                TI_C54X_LONG32_PAIR_STATUS_WAIT);
            result.bus_status = TI_C54X_BUS_OK;
            return result;
        }
        return ti_c54x_long32_pair_invalid_completion(
            TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
    }
    if (transaction->state != TI_C54X_LONG32_PAIR_STATE_COMPLETION_READY) {
        return ti_c54x_long32_pair_invalid_completion(
            TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
    }
    result.status = TI_C54X_LONG32_PAIR_STATUS_OK;
    result.first_msw = transaction->staged_first_msw;
    result.second_lsw = transaction->staged_second_lsw;
    result.bus_status = transaction->staged_bus_status;
    transaction->state = TI_C54X_LONG32_PAIR_STATE_COMPLETE;
    return result;
}

ti_c54x_long32_pair_status ti_c54x_long32_pair_transaction_reset(
    ti_c54x_long32_pair_transaction *transaction)
{
    ti_c54x_bus_word16_transaction_status status;

    if (transaction == NULL) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_long32_pair_transaction_is_canonical(transaction)) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
    if (transaction->state != TI_C54X_LONG32_PAIR_STATE_IDLE &&
        transaction->state != TI_C54X_LONG32_PAIR_STATE_COMPLETE &&
        transaction->state != TI_C54X_LONG32_PAIR_STATE_FAULT) {
        return TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE;
    }
    status = ti_c54x_bus_word16_transaction_reset(
        &transaction->word16_transaction);
    if (status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return ti_c54x_long32_pair_map_status(status);
    }
    ti_c54x_long32_pair_clear(transaction);
    return TI_C54X_LONG32_PAIR_STATUS_OK;
}
