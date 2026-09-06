/*
 * Module contract
 *
 * Owns: lifecycle reduction for one caller-owned WORD16 request, including
 * one issue authorization, resumable WAIT observations, one staged terminal
 * observation, and one completion publication.
 * Does not own: callbacks, physical memory or visibility, arbitration, device
 * timing, address-effect phases, CPU state, public ABI, observers, diagnostics,
 * environment, or globals. Active state may be pointer-aliased by the caller
 * under synchronization, but no second authoritative value may be created.
 */

#include "word16_transaction.h"

#include "word16_transaction_validate.h"

#include <stddef.h>

static ti_c54x_bus_word16_transaction_issue_result
ti_c54x_bus_word16_transaction_invalid_issue(
    ti_c54x_bus_word16_transaction_status status)
{
    ti_c54x_bus_word16_transaction_issue_result result = {0};

    result.status = status;
    return result;
}

static ti_c54x_bus_word16_transaction_completion_result
ti_c54x_bus_word16_transaction_invalid_completion(
    ti_c54x_bus_word16_transaction_status status)
{
    ti_c54x_bus_word16_transaction_completion_result result = {0};

    result.status = status;
    return result;
}

static bool ti_c54x_bus_word16_transaction_is_usable(
    const ti_c54x_bus_word16_transaction *transaction)
{
    return transaction != NULL &&
           ti_c54x_bus_word16_transaction_is_canonical(transaction);
}

static void ti_c54x_bus_word16_transaction_clear(
    ti_c54x_bus_word16_transaction *transaction)
{
    transaction->state = TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE;
    transaction->transaction_id = UINT64_C(0);
    transaction->request.space =
        TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID;
    transaction->request.access =
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID;
    transaction->request.address_word = UINT32_C(0);
    transaction->request.write_value_word = UINT16_C(0);
    transaction->staged_read_value_word = UINT16_C(0);
    transaction->staged_bus_status = TI_C54X_BUS_OK;
}

ti_c54x_bus_word16_transaction_status
ti_c54x_bus_word16_transaction_prepare(
    ti_c54x_bus_word16_transaction *transaction,
    const ti_c54x_bus_word16_request *request,
    uint64_t transaction_id)
{
    if (transaction == NULL || request == NULL) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_bus_word16_transaction_is_canonical(transaction) ||
        transaction->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE;
    }
    if (transaction_id == UINT64_C(0) ||
        !ti_c54x_bus_word16_transaction_request_is_valid(request)) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT;
    }

    transaction->transaction_id = transaction_id;
    transaction->request = *request;
    transaction->staged_read_value_word = UINT16_C(0);
    transaction->staged_bus_status = TI_C54X_BUS_OK;
    transaction->state = TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY;
    return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK;
}

ti_c54x_bus_word16_transaction_issue_result
ti_c54x_bus_word16_transaction_begin(
    ti_c54x_bus_word16_transaction *transaction)
{
    ti_c54x_bus_word16_transaction_issue_result result;

    if (transaction == NULL) {
        return ti_c54x_bus_word16_transaction_invalid_issue(
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    }
    if (!ti_c54x_bus_word16_transaction_is_usable(transaction) ||
        transaction->state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY) {
        return ti_c54x_bus_word16_transaction_invalid_issue(
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    }

    result.status = TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK;
    result.transaction_id = transaction->transaction_id;
    result.request = transaction->request;
    transaction->state = TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT;
    return result;
}

ti_c54x_bus_word16_transaction_status
ti_c54x_bus_word16_transaction_poll(
    ti_c54x_bus_word16_transaction *transaction,
    const ti_c54x_bus_word16_transaction_observation *observation)
{
    if (transaction == NULL || observation == NULL) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_bus_word16_transaction_is_usable(transaction) ||
        transaction->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE;
    }
    if (observation->transaction_id == UINT64_C(0)) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT;
    }
    if (observation->transaction_id != transaction->transaction_id) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION;
    }
    if (!ti_c54x_bus_word16_transaction_observation_is_valid(observation)) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT;
    }
    if (observation->kind ==
            TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_READY &&
        transaction->request.access ==
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
        observation->read_value_word != UINT16_C(0)) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT;
    }
    if (observation->kind ==
        TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_WAIT) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT;
    }

    transaction->staged_read_value_word = observation->read_value_word;
    transaction->staged_bus_status = observation->bus_status;
    transaction->state =
        TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETION_READY;
    return observation->kind ==
                   TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_FAULT
               ? TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT
               : TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK;
}

ti_c54x_bus_word16_transaction_completion_result
ti_c54x_bus_word16_transaction_complete(
    ti_c54x_bus_word16_transaction *transaction,
    uint64_t transaction_id)
{
    ti_c54x_bus_word16_transaction_completion_result result;

    if (transaction == NULL) {
        return ti_c54x_bus_word16_transaction_invalid_completion(
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    }
    if (!ti_c54x_bus_word16_transaction_is_usable(transaction)) {
        return ti_c54x_bus_word16_transaction_invalid_completion(
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    }
    if (transaction->state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT &&
        transaction->state !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETION_READY) {
        return ti_c54x_bus_word16_transaction_invalid_completion(
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    }
    if (transaction_id == UINT64_C(0)) {
        return ti_c54x_bus_word16_transaction_invalid_completion(
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    }
    if (transaction_id != transaction->transaction_id) {
        return ti_c54x_bus_word16_transaction_invalid_completion(
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION);
    }
    if (transaction->state ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
        result = ti_c54x_bus_word16_transaction_invalid_completion(
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT);
        return result;
    }
    if (transaction->staged_bus_status != TI_C54X_BUS_OK) {
        result = ti_c54x_bus_word16_transaction_invalid_completion(
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT);
        result.bus_status = transaction->staged_bus_status;
        transaction->state = TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT;
        return result;
    }

    result = ti_c54x_bus_word16_transaction_invalid_completion(
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    result.bus_status = TI_C54X_BUS_OK;
    if (transaction->request.access ==
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        result.read_value_word = transaction->staged_read_value_word;
    }
    transaction->state = TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE;
    return result;
}

ti_c54x_bus_word16_transaction_status
ti_c54x_bus_word16_transaction_reset(
    ti_c54x_bus_word16_transaction *transaction)
{
    if (transaction == NULL) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_bus_word16_transaction_is_usable(transaction)) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE;
    }
    if (transaction->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE &&
        transaction->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE &&
        transaction->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT) {
        return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE;
    }

    ti_c54x_bus_word16_transaction_clear(transaction);
    return TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK;
}
