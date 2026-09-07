/*
 * Module contract
 *
 * Owns: read-only validation of WORD16 transaction states, requests, and
 * observations before lifecycle mutation.
 * Does not own: transaction transitions, callbacks, memory, arbitration,
 * timing, CPU state, diagnostics, or public ABI.
 */

#include "word16_transaction_validate.h"

#include <stddef.h>

bool ti_c54x_bus_word16_transaction_state_is_valid(
    ti_c54x_bus_word16_transaction_state state)
{
    return state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE ||
           state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY ||
           state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT ||
           state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETION_READY ||
           state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE ||
           state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT;
}

bool ti_c54x_bus_word16_transaction_is_canonical(
    const ti_c54x_bus_word16_transaction *transaction)
{
    if (transaction == NULL ||
        !ti_c54x_bus_word16_transaction_state_is_valid(transaction->state)) {
        return false;
    }
    if (transaction->state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE) {
        return transaction->transaction_id == UINT64_C(0) &&
               transaction->request.space ==
                   TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID &&
               transaction->request.access ==
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID &&
               transaction->request.address_word == UINT32_C(0) &&
               transaction->request.write_value_word == UINT16_C(0) &&
               transaction->staged_read_value_word == UINT16_C(0) &&
               transaction->staged_bus_status == TI_C54X_BUS_OK;
    }
    if (transaction->transaction_id == UINT64_C(0) ||
        !ti_c54x_bus_word16_transaction_request_is_valid(
            &transaction->request)) {
        return false;
    }
    if (transaction->state ==
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY ||
        transaction->state ==
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT) {
        return transaction->staged_read_value_word == UINT16_C(0) &&
               transaction->staged_bus_status == TI_C54X_BUS_OK;
    }
    if (transaction->state ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETION_READY) {
        if (transaction->staged_bus_status == TI_C54X_BUS_OK) {
            return transaction->request.access !=
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ||
                   transaction->staged_read_value_word == UINT16_C(0);
        }
        return transaction->staged_read_value_word == UINT16_C(0) &&
               ti_c54x_bus_word16_transaction_fault_status_is_valid(
                   transaction->staged_bus_status);
    }
    if (transaction->state ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE) {
        return transaction->staged_bus_status == TI_C54X_BUS_OK &&
               (transaction->request.access !=
                    TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ||
                transaction->staged_read_value_word == UINT16_C(0));
    }
    return transaction->staged_read_value_word == UINT16_C(0) &&
           ti_c54x_bus_word16_transaction_fault_status_is_valid(
               transaction->staged_bus_status);
}

bool ti_c54x_bus_word16_transaction_request_is_valid(
    const ti_c54x_bus_word16_request *request)
{
    return ti_c54x_bus_word16_request_is_valid(request);
}

bool ti_c54x_bus_word16_transaction_fault_status_is_valid(
    ti_c54x_bus_status status)
{
    return status == TI_C54X_BUS_UNMAPPED || status == TI_C54X_BUS_FAULT ||
           status == TI_C54X_BUS_NO_CALLBACK;
}

bool ti_c54x_bus_word16_observation_is_valid(
    const ti_c54x_bus_word16_observation *observation)
{
    if (observation == NULL || observation->transaction_id == UINT64_C(0)) {
        return false;
    }
    if (observation->kind ==
            TI_C54X_BUS_WORD16_OBSERVATION_WAIT) {
        return observation->bus_status == TI_C54X_BUS_OK &&
               observation->read_value_word == UINT16_C(0);
    }
    if (observation->kind ==
            TI_C54X_BUS_WORD16_OBSERVATION_READY) {
        return observation->bus_status == TI_C54X_BUS_OK;
    }
    if (observation->kind ==
            TI_C54X_BUS_WORD16_OBSERVATION_FAULT) {
        return observation->read_value_word == UINT16_C(0) &&
               ti_c54x_bus_word16_transaction_fault_status_is_valid(
                   observation->bus_status);
    }
    return false;
}
