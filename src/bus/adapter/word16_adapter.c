/*
 * Module contract
 *
 * Owns: one callback invocation per accepted BEGIN/RESUME phase and one
 * reducer transition for each canonical endpoint observation.
 * Does not own: endpoint storage, physical memory, mapping, arbitration,
 * byte order, CPU state, instruction semantics, timing, allocation,
 * diagnostics, or environment reads.
 */

#include "word16_adapter.h"

#include "../transaction/word16_transaction_validate.h"

#include <stdbool.h>
#include <stddef.h>

static ti_c54x_bus_word16_adapter_result result_with_status(
    ti_c54x_bus_word16_adapter_status status,
    ti_c54x_bus_status bus_status)
{
    ti_c54x_bus_word16_adapter_result result = {0};

    result.status = status;
    result.bus_status = bus_status;
    return result;
}

static bool endpoint_is_absent(const ti_c54x_bus_word16_endpoint *endpoint)
{
    return endpoint != NULL && endpoint->context == NULL &&
           endpoint->advance == NULL && endpoint->cancel == NULL;
}

static bool endpoint_is_partial(const ti_c54x_bus_word16_endpoint *endpoint)
{
    if (endpoint == NULL || endpoint_is_absent(endpoint)) {
        return false;
    }
    return endpoint->advance == NULL || endpoint->cancel == NULL;
}

static bool endpoint_is_valid(const ti_c54x_bus_word16_endpoint *endpoint)
{
    return endpoint != NULL && !endpoint_is_absent(endpoint) &&
           !endpoint_is_partial(endpoint);
}

ti_c54x_bus_word16_adapter_status
ti_c54x_bus_word16_adapter_validate_endpoint(
    const ti_c54x_bus_word16_endpoint *endpoint)
{
    if (endpoint == NULL) {
        return TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT;
    }
    if (endpoint_is_absent(endpoint)) {
        return TI_C54X_BUS_WORD16_ADAPTER_STATUS_NO_CALLBACK;
    }
    if (!endpoint_is_valid(endpoint)) {
        return TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ENDPOINT;
    }
    return TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK;
}

static ti_c54x_bus_word16_adapter_result map_transaction_status(
    ti_c54x_bus_word16_transaction_status status,
    ti_c54x_bus_status bus_status,
    uint16_t read_value_word)
{
    ti_c54x_bus_word16_adapter_result result = {0};

    result.read_value_word = read_value_word;
    result.bus_status = bus_status;
    switch (status) {
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK:
        result.status = TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK;
        break;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT:
        result.status = TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT;
        break;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT:
        result.status = TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT;
        break;
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION:
    case TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT:
        result.status = TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_OBSERVATION;
        result.read_value_word = UINT16_C(0);
        result.bus_status = TI_C54X_BUS_OK;
        break;
    default:
        result.status = TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT;
        result.read_value_word = UINT16_C(0);
        result.bus_status = TI_C54X_BUS_OK;
        break;
    }
    return result;
}

static ti_c54x_bus_word16_adapter_result invoke(
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_bus_word16_advance_phase phase,
    ti_c54x_bus_word16_transaction *transaction)
{
    ti_c54x_bus_word16_transaction_issue_result issue;
    ti_c54x_bus_word16_observation observation = {0};
    ti_c54x_bus_word16_transaction_status status;
    ti_c54x_bus_word16_transaction_completion_result completion;

    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        issue = ti_c54x_bus_word16_transaction_begin(transaction);
        if (issue.status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return result_with_status(
                TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT,
                TI_C54X_BUS_OK);
        }
    }
    endpoint->advance(endpoint->context, phase, transaction->transaction_id,
                      &transaction->request, &observation);
    status = ti_c54x_bus_word16_transaction_poll(transaction, &observation);
    if (status == TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT) {
        return result_with_status(TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT,
                                  TI_C54X_BUS_OK);
    }
    if (status == TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT) {
        completion = ti_c54x_bus_word16_transaction_complete(
            transaction, transaction->transaction_id);
        return map_transaction_status(completion.status,
                                      completion.bus_status,
                                      completion.read_value_word);
    }
    if (status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return result_with_status(
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_OBSERVATION,
            TI_C54X_BUS_OK);
    }
    completion = ti_c54x_bus_word16_transaction_complete(
        transaction, transaction->transaction_id);
    return map_transaction_status(completion.status, completion.bus_status,
                                  completion.read_value_word);
}

ti_c54x_bus_word16_adapter_result ti_c54x_bus_word16_adapter_advance(
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_bus_word16_advance_phase phase,
    ti_c54x_bus_word16_transaction *transaction)
{
    if (endpoint == NULL || transaction == NULL ||
        (phase != TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
         phase != TI_C54X_BUS_WORD16_ADVANCE_RESUME)) {
        return result_with_status(TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT,
                                  TI_C54X_BUS_OK);
    }
    {
        const ti_c54x_bus_word16_adapter_status endpoint_status =
            ti_c54x_bus_word16_adapter_validate_endpoint(endpoint);
        if (endpoint_status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_NO_CALLBACK) {
            return result_with_status(endpoint_status, TI_C54X_BUS_NO_CALLBACK);
        }
        if (endpoint_status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            return result_with_status(endpoint_status, TI_C54X_BUS_OK);
        }
    }
    if (!ti_c54x_bus_word16_transaction_is_canonical(transaction) ||
        transaction->request.space !=
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA) {
        return result_with_status(
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT, TI_C54X_BUS_OK);
    }
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
        transaction->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY) {
        return result_with_status(
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT, TI_C54X_BUS_OK);
    }
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_RESUME &&
        transaction->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
        return result_with_status(
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT, TI_C54X_BUS_OK);
    }
    return invoke(endpoint, phase, transaction);
}

ti_c54x_bus_word16_adapter_result ti_c54x_bus_word16_adapter_cancel(
    const ti_c54x_bus_word16_endpoint *endpoint,
    ti_c54x_bus_word16_transaction *transaction)
{
    ti_c54x_bus_word16_cancel_result cancel_result;
    ti_c54x_bus_word16_transaction_status status;

    if (endpoint == NULL || transaction == NULL) {
        return result_with_status(TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT,
                                  TI_C54X_BUS_OK);
    }
    {
        const ti_c54x_bus_word16_adapter_status endpoint_status =
            ti_c54x_bus_word16_adapter_validate_endpoint(endpoint);
        if (endpoint_status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_NO_CALLBACK) {
            return result_with_status(endpoint_status, TI_C54X_BUS_NO_CALLBACK);
        }
        if (endpoint_status != TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK) {
            return result_with_status(endpoint_status, TI_C54X_BUS_OK);
        }
    }
    if (!ti_c54x_bus_word16_transaction_is_canonical(transaction) ||
        transaction->state != TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
        return result_with_status(
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT, TI_C54X_BUS_OK);
    }
    cancel_result = endpoint->cancel(endpoint->context,
                                     transaction->transaction_id,
                                     &transaction->request);
    if (cancel_result != TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED) {
        return result_with_status(
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_CANCEL_FAILED, TI_C54X_BUS_OK);
    }
    status = ti_c54x_bus_word16_transaction_cancel(
        transaction, transaction->transaction_id);
    if (status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        return result_with_status(
            TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT, TI_C54X_BUS_OK);
    }
    return result_with_status(TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK,
                              TI_C54X_BUS_OK);
}
