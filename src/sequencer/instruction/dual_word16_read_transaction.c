/*
 * Module contract
 *
 * Owns: the allocation-free state reducer for two copied WORD16 READ
 * requests, including deterministic X/Y issue order and staged pair
 * publication after both children complete.
 * Does not own: endpoint callbacks, physical memory or visibility, bus
 * arbitration, timing, CPU state, opcode or effect publication, observers,
 * diagnostics, environment, globals, heap, or I/O. The one child
 * transaction is reused for Y after X completes.
 */

#include "dual_word16_read_transaction.h"

#include <stddef.h>

#include "bus/transaction/word16_transaction_validate.h"

static ti_c54x_dual_word16_read_status map_status(
    ti_c54x_bus_word16_transaction_status status)
{
    switch (status) {
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK:
        return TI_C54X_DUAL_WORD16_READ_STATUS_OK;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT:
        return TI_C54X_DUAL_WORD16_READ_STATUS_WAIT;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT:
        return TI_C54X_DUAL_WORD16_READ_STATUS_FAULT;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT:
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_ARGUMENT;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE:
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION:
        return TI_C54X_DUAL_WORD16_READ_STATUS_STALE_TRANSACTION;
    default:
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE;
    }
}

static ti_c54x_dual_word16_read_issue_result invalid_issue(
    ti_c54x_dual_word16_read_status status)
{
    ti_c54x_dual_word16_read_issue_result result = {0};

    result.status = status;
    return result;
}

static ti_c54x_dual_word16_read_completion_result invalid_completion(
    ti_c54x_dual_word16_read_status status)
{
    ti_c54x_dual_word16_read_completion_result result = {0};

    result.status = status;
    return result;
}

static void clear_transaction(
    ti_c54x_dual_word16_read_transaction *transaction)
{
    *transaction = (ti_c54x_dual_word16_read_transaction){0};
    transaction->staged_bus_status = TI_C54X_BUS_OK;
}

static bool state_is_valid(ti_c54x_dual_word16_read_state state)
{
    return state == TI_C54X_DUAL_WORD16_READ_STATE_IDLE ||
           state == TI_C54X_DUAL_WORD16_READ_STATE_X ||
           state == TI_C54X_DUAL_WORD16_READ_STATE_Y ||
           state == TI_C54X_DUAL_WORD16_READ_STATE_COMPLETION_READY ||
           state == TI_C54X_DUAL_WORD16_READ_STATE_COMPLETE ||
           state == TI_C54X_DUAL_WORD16_READ_STATE_FAULT;
}

static ti_c54x_bus_word16_request snapshot_request(
    const ti_c54x_dual_word16_read_request_snapshot *snapshot, bool y)
{
    ti_c54x_bus_word16_request request = {0};

    request.space = TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA;
    request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    request.address_word =
        y ? snapshot->y_request.address_word : snapshot->x_request.address_word;
    request.write_value_word = UINT16_C(0);
    return request;
}

bool ti_c54x_dual_word16_read_request_is_valid(
    const ti_c54x_dual_word16_read_request *request)
{
    if (request == NULL ||
        !ti_c54x_bus_word16_request_is_valid(&request->x_request) ||
        !ti_c54x_bus_word16_request_is_valid(&request->y_request) ||
        request->x_request.space !=
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        request->y_request.space !=
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA ||
        request->x_request.access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        request->y_request.access != TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
        request->x_transaction_id == UINT64_C(0) ||
        request->y_transaction_id == UINT64_C(0) ||
        request->y_transaction_id <= request->x_transaction_id) {
        return false;
    }
    return true;
}

static bool child_matches_slot(
    const ti_c54x_dual_word16_read_transaction *transaction,
    bool y)
{
    const ti_c54x_bus_word16_request expected_request =
        snapshot_request(&transaction->request, y);
    const uint64_t expected_id =
        y ? transaction->request.y_transaction_id
          : transaction->request.x_transaction_id;
    const ti_c54x_bus_word16_request *actual_request =
        &transaction->word16_transaction.request;
    return transaction->word16_transaction.transaction_id == expected_id &&
           actual_request->space == expected_request.space &&
           actual_request->access == expected_request.access &&
           actual_request->address_word == expected_request.address_word &&
           actual_request->write_value_word ==
               expected_request.write_value_word;
}

static bool active_child_is_canonical(
    const ti_c54x_dual_word16_read_transaction *transaction,
    bool y)
{
    const ti_c54x_bus_word16_transaction_state state =
        transaction->word16_transaction.state;

    return ti_c54x_bus_word16_transaction_is_canonical(
               &transaction->word16_transaction) &&
           child_matches_slot(transaction, y) &&
           (state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY ||
            state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT);
}

bool ti_c54x_dual_word16_read_transaction_is_canonical(
    const ti_c54x_dual_word16_read_transaction *transaction)
{
    const ti_c54x_bus_word16_transaction_state child_state =
        transaction == NULL ? TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE
                            : transaction->word16_transaction.state;

    if (transaction == NULL || !state_is_valid(transaction->state) ||
        !ti_c54x_bus_word16_transaction_is_canonical(
            &transaction->word16_transaction)) {
        return false;
    }
    if (transaction->state == TI_C54X_DUAL_WORD16_READ_STATE_IDLE) {
        return child_state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE &&
               transaction->request.x_request.address_word == UINT32_C(0) &&
               transaction->request.y_request.address_word == UINT32_C(0) &&
               transaction->request.x_transaction_id == UINT64_C(0) &&
               transaction->request.y_transaction_id == UINT64_C(0) &&
               transaction->staged_x_value_word == UINT16_C(0) &&
               transaction->staged_y_value_word == UINT16_C(0) &&
               transaction->staged_bus_status == TI_C54X_BUS_OK;
    }
    if (transaction->request.x_request.address_word > UINT16_MAX ||
        transaction->request.y_request.address_word > UINT16_MAX ||
        transaction->request.x_transaction_id == UINT64_C(0) ||
        transaction->request.y_transaction_id == UINT64_C(0) ||
        transaction->request.y_transaction_id <=
            transaction->request.x_transaction_id) {
        return false;
    }
    if (transaction->state == TI_C54X_DUAL_WORD16_READ_STATE_X ||
        transaction->state == TI_C54X_DUAL_WORD16_READ_STATE_Y) {
        const bool y = transaction->state == TI_C54X_DUAL_WORD16_READ_STATE_Y;

        return transaction->staged_bus_status == TI_C54X_BUS_OK &&
               transaction->staged_y_value_word == UINT16_C(0) &&
               (!y ? transaction->staged_x_value_word == UINT16_C(0) : true) &&
               active_child_is_canonical(transaction, y);
    }
    if (transaction->state ==
            TI_C54X_DUAL_WORD16_READ_STATE_COMPLETION_READY ||
        transaction->state == TI_C54X_DUAL_WORD16_READ_STATE_COMPLETE) {
        return transaction->staged_bus_status == TI_C54X_BUS_OK &&
               child_state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE &&
               child_matches_slot(transaction, true);
    }
    return child_state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT &&
           ti_c54x_bus_word16_transaction_fault_status_is_valid(
               transaction->staged_bus_status) &&
           transaction->staged_x_value_word == UINT16_C(0) &&
           transaction->staged_y_value_word == UINT16_C(0) &&
           (child_matches_slot(transaction, false) ||
            child_matches_slot(transaction, true));
}

ti_c54x_dual_word16_read_status
ti_c54x_dual_word16_read_transaction_prepare(
    ti_c54x_dual_word16_read_transaction *transaction,
    const ti_c54x_dual_word16_read_request *request)
{
    if (transaction == NULL || request == NULL) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_dual_word16_read_transaction_is_canonical(transaction) ||
        transaction->state != TI_C54X_DUAL_WORD16_READ_STATE_IDLE) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE;
    }
    if (!ti_c54x_dual_word16_read_request_is_valid(request)) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (ti_c54x_bus_word16_transaction_prepare(
            &transaction->word16_transaction, &request->x_request,
            request->x_transaction_id) !=
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE;
    }
    transaction->request.x_request.address_word = request->x_request.address_word;
    transaction->request.y_request.address_word = request->y_request.address_word;
    transaction->request.x_transaction_id = request->x_transaction_id;
    transaction->request.y_transaction_id = request->y_transaction_id;
    transaction->staged_x_value_word = UINT16_C(0);
    transaction->staged_y_value_word = UINT16_C(0);
    transaction->staged_bus_status = TI_C54X_BUS_OK;
    transaction->state = TI_C54X_DUAL_WORD16_READ_STATE_X;
    return TI_C54X_DUAL_WORD16_READ_STATUS_OK;
}

ti_c54x_dual_word16_read_issue_result
ti_c54x_dual_word16_read_transaction_begin(
    ti_c54x_dual_word16_read_transaction *transaction)
{
    ti_c54x_bus_word16_transaction_issue_result child;
    ti_c54x_dual_word16_read_issue_result result;

    if (transaction == NULL) {
        return invalid_issue(
            TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_ARGUMENT);
    }
    if (!ti_c54x_dual_word16_read_transaction_is_canonical(transaction) ||
        (transaction->state != TI_C54X_DUAL_WORD16_READ_STATE_X &&
         transaction->state != TI_C54X_DUAL_WORD16_READ_STATE_Y)) {
        return invalid_issue(TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE);
    }
    child = ti_c54x_bus_word16_transaction_begin(
        &transaction->word16_transaction);
    if (child.status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return invalid_issue(map_status(child.status));
    }
    result.status = TI_C54X_DUAL_WORD16_READ_STATUS_OK;
    result.transaction_id = child.transaction_id;
    result.request = child.request;
    return result;
}

static ti_c54x_dual_word16_read_status finish_child(
    ti_c54x_dual_word16_read_transaction *transaction,
    bool y)
{
    ti_c54x_bus_word16_transaction_completion_result completion;

    completion = ti_c54x_bus_word16_transaction_complete(
        &transaction->word16_transaction,
        transaction->word16_transaction.transaction_id);
    if (completion.status == TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT) {
        transaction->staged_x_value_word = UINT16_C(0);
        transaction->staged_y_value_word = UINT16_C(0);
        transaction->staged_bus_status = completion.bus_status;
        transaction->state = TI_C54X_DUAL_WORD16_READ_STATE_FAULT;
        return TI_C54X_DUAL_WORD16_READ_STATUS_FAULT;
    }
    if (completion.status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return map_status(completion.status);
    }
    if (y) {
        transaction->staged_y_value_word = completion.read_value_word;
        transaction->staged_bus_status = completion.bus_status;
        transaction->state = TI_C54X_DUAL_WORD16_READ_STATE_COMPLETION_READY;
        return TI_C54X_DUAL_WORD16_READ_STATUS_OK;
    }
    transaction->staged_x_value_word = completion.read_value_word;
    {
        const ti_c54x_bus_word16_request y_request =
            snapshot_request(&transaction->request, true);
        if (ti_c54x_bus_word16_transaction_reset(
                &transaction->word16_transaction) !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK ||
            ti_c54x_bus_word16_transaction_prepare(
                &transaction->word16_transaction, &y_request,
                transaction->request.y_transaction_id) !=
                TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            clear_transaction(transaction);
            return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE;
        }
    }
    transaction->state = TI_C54X_DUAL_WORD16_READ_STATE_Y;
    return TI_C54X_DUAL_WORD16_READ_STATUS_OK;
}

ti_c54x_dual_word16_read_status ti_c54x_dual_word16_read_transaction_poll(
    ti_c54x_dual_word16_read_transaction *transaction,
    const ti_c54x_bus_word16_observation *observation)
{
    const bool y = transaction != NULL &&
                   transaction->state == TI_C54X_DUAL_WORD16_READ_STATE_Y;
    ti_c54x_bus_word16_transaction_status child_status;
    ti_c54x_dual_word16_read_status status;

    if (transaction == NULL || observation == NULL) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_dual_word16_read_transaction_is_canonical(transaction) ||
        (transaction->state != TI_C54X_DUAL_WORD16_READ_STATE_X &&
         transaction->state != TI_C54X_DUAL_WORD16_READ_STATE_Y)) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE;
    }
    child_status = ti_c54x_bus_word16_transaction_poll(
        &transaction->word16_transaction, observation);
    status = map_status(child_status);
    if (status != TI_C54X_DUAL_WORD16_READ_STATUS_OK &&
        status != TI_C54X_DUAL_WORD16_READ_STATUS_FAULT) {
        return status;
    }
    return finish_child(transaction, y);
}

ti_c54x_dual_word16_read_status ti_c54x_dual_word16_read_transaction_cancel(
    ti_c54x_dual_word16_read_transaction *transaction,
    uint64_t transaction_id)
{
    ti_c54x_bus_word16_transaction_status status;

    if (transaction == NULL) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_dual_word16_read_transaction_is_canonical(transaction) ||
        (transaction->state != TI_C54X_DUAL_WORD16_READ_STATE_X &&
         transaction->state != TI_C54X_DUAL_WORD16_READ_STATE_Y)) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE;
    }
    if (transaction->word16_transaction.state !=
        TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE;
    }
    if (transaction_id == UINT64_C(0)) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (transaction_id != transaction->word16_transaction.transaction_id) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_STALE_TRANSACTION;
    }
    status = ti_c54x_bus_word16_transaction_cancel(
        &transaction->word16_transaction, transaction_id);
    if (status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return map_status(status);
    }
    clear_transaction(transaction);
    return TI_C54X_DUAL_WORD16_READ_STATUS_OK;
}

ti_c54x_dual_word16_read_status
ti_c54x_dual_word16_read_transaction_abandon_staged(
    ti_c54x_dual_word16_read_transaction *transaction)
{
    const bool staged_y =
        transaction != NULL &&
        transaction->state == TI_C54X_DUAL_WORD16_READ_STATE_Y &&
        transaction->word16_transaction.state ==
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY;
    const bool completion_ready =
        transaction != NULL &&
        transaction->state ==
            TI_C54X_DUAL_WORD16_READ_STATE_COMPLETION_READY &&
        transaction->word16_transaction.state ==
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE;

    if (transaction == NULL) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_dual_word16_read_transaction_is_canonical(transaction) ||
        (!staged_y && !completion_ready)) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE;
    }
    clear_transaction(transaction);
    return TI_C54X_DUAL_WORD16_READ_STATUS_OK;
}

ti_c54x_dual_word16_read_completion_result
ti_c54x_dual_word16_read_transaction_complete(
    ti_c54x_dual_word16_read_transaction *transaction)
{
    ti_c54x_dual_word16_read_completion_result result;

    if (transaction == NULL) {
        return invalid_completion(
            TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_ARGUMENT);
    }
    if (!ti_c54x_dual_word16_read_transaction_is_canonical(transaction)) {
        return invalid_completion(
            TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE);
    }
    if (transaction->state == TI_C54X_DUAL_WORD16_READ_STATE_FAULT) {
        result = invalid_completion(TI_C54X_DUAL_WORD16_READ_STATUS_FAULT);
        result.bus_status = transaction->staged_bus_status;
        return result;
    }
    if (transaction->state == TI_C54X_DUAL_WORD16_READ_STATE_X ||
        transaction->state == TI_C54X_DUAL_WORD16_READ_STATE_Y) {
        if (transaction->word16_transaction.state ==
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
            return invalid_completion(TI_C54X_DUAL_WORD16_READ_STATUS_WAIT);
        }
        return invalid_completion(
            TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE);
    }
    if (transaction->state !=
        TI_C54X_DUAL_WORD16_READ_STATE_COMPLETION_READY) {
        return invalid_completion(
            TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE);
    }
    result.status = TI_C54X_DUAL_WORD16_READ_STATUS_OK;
    result.x_value_word = transaction->staged_x_value_word;
    result.y_value_word = transaction->staged_y_value_word;
    result.bus_status = transaction->staged_bus_status;
    transaction->state = TI_C54X_DUAL_WORD16_READ_STATE_COMPLETE;
    return result;
}

ti_c54x_dual_word16_read_status ti_c54x_dual_word16_read_transaction_reset(
    ti_c54x_dual_word16_read_transaction *transaction)
{
    ti_c54x_bus_word16_transaction_status status;

    if (transaction == NULL) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_dual_word16_read_transaction_is_canonical(transaction)) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE;
    }
    if (transaction->state != TI_C54X_DUAL_WORD16_READ_STATE_IDLE &&
        transaction->state != TI_C54X_DUAL_WORD16_READ_STATE_COMPLETE &&
        transaction->state != TI_C54X_DUAL_WORD16_READ_STATE_FAULT) {
        return TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE;
    }
    status = ti_c54x_bus_word16_transaction_reset(
        &transaction->word16_transaction);
    if (status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return map_status(status);
    }
    clear_transaction(transaction);
    return TI_C54X_DUAL_WORD16_READ_STATUS_OK;
}
