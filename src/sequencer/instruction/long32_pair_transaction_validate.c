/*
 * Module contract
 *
 * Owns: read-only validation and canonical WORD16 request construction for
 * the private LONG32 pair reducer.
 * Does not own: lifecycle transitions, endpoint callbacks, physical memory,
 * CPU state, publication, timing, observers, diagnostics, or global state.
 */

#include "long32_pair_transaction_validate.h"

#include <stddef.h>

#include "bus/request/word16_request.h"
#include "bus/transaction/word16_transaction_validate.h"

static bool ti_c54x_long32_pair_state_is_valid(
    ti_c54x_long32_pair_state state)
{
    return state == TI_C54X_LONG32_PAIR_STATE_IDLE ||
           state == TI_C54X_LONG32_PAIR_STATE_FIRST ||
           state == TI_C54X_LONG32_PAIR_STATE_SECOND ||
           state == TI_C54X_LONG32_PAIR_STATE_COMPLETION_READY ||
           state == TI_C54X_LONG32_PAIR_STATE_COMPLETE ||
           state == TI_C54X_LONG32_PAIR_STATE_FAULT;
}

static bool ti_c54x_long32_pair_request_access_is_valid(
    ti_c54x_bus_word16_request_access access)
{
    return access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ ||
           access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
}

bool ti_c54x_long32_pair_request_is_valid(
    const ti_c54x_long32_pair_request *request)
{
    if (request == NULL ||
        request->operand.width != TI_C54X_DATA_OPERAND_WIDTH_LONG32 ||
        !ti_c54x_data_operand_is_valid(&request->operand) ||
        request->operand.first_word == request->operand.second_word ||
        !ti_c54x_long32_pair_request_access_is_valid(request->access) ||
        request->first_transaction_id == UINT64_C(0) ||
        request->second_transaction_id == UINT64_C(0) ||
        request->second_transaction_id <= request->first_transaction_id) {
        return false;
    }
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
        (request->first_value_msw != UINT16_C(0) ||
         request->second_value_lsw != UINT16_C(0))) {
        return false;
    }
    return true;
}

bool ti_c54x_long32_pair_build_word16_request(
    const ti_c54x_long32_pair_request *request,
    bool second,
    ti_c54x_bus_word16_request *out_request)
{
    ti_c54x_bus_word16_request_result built;
    ti_c54x_data_word_address address;
    uint16_t value;

    if (request == NULL || out_request == NULL) {
        return false;
    }
    address = second ? request->operand.second_word : request->operand.first_word;
    value = second ? request->second_value_lsw : request->first_value_msw;
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        built = ti_c54x_bus_word16_request_read(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, address);
    } else if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        built = ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, address, value);
    } else {
        return false;
    }
    if (built.status != TI_C54X_BUS_WORD16_REQUEST_STATUS_OK) {
        return false;
    }
    *out_request = built.request;
    return true;
}

static bool ti_c54x_long32_pair_child_matches_slot(
    const ti_c54x_long32_pair_transaction *transaction,
    bool second)
{
    const uint64_t expected_id =
        second ? transaction->request.second_transaction_id
               : transaction->request.first_transaction_id;
    const uint32_t expected_address = second
                                          ? transaction->request.operand.second_word
                                          : transaction->request.operand.first_word;
    const uint16_t expected_value =
        transaction->request.access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
            ? UINT16_C(0)
            : (second ? transaction->request.second_value_lsw
                       : transaction->request.first_value_msw);

    return transaction->word16_transaction.transaction_id == expected_id &&
           transaction->word16_transaction.request.space ==
               TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
           transaction->word16_transaction.request.access ==
               transaction->request.access &&
           transaction->word16_transaction.request.address_word ==
               expected_address &&
           transaction->word16_transaction.request.write_value_word ==
               expected_value;
}

static bool ti_c54x_long32_pair_active_child_is_canonical(
    const ti_c54x_long32_pair_transaction *transaction,
    bool second)
{
    const ti_c54x_bus_word16_transaction_state state =
        transaction->word16_transaction.state;

    if (!ti_c54x_bus_word16_transaction_is_canonical(
            &transaction->word16_transaction) ||
        !ti_c54x_long32_pair_child_matches_slot(transaction, second)) {
        return false;
    }
    return state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY ||
           state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT;
}

bool ti_c54x_long32_pair_transaction_is_canonical(
    const ti_c54x_long32_pair_transaction *transaction)
{
    ti_c54x_bus_word16_transaction_state child_state;

    if (transaction == NULL ||
        !ti_c54x_long32_pair_state_is_valid(transaction->state) ||
        !ti_c54x_bus_word16_transaction_is_canonical(
            &transaction->word16_transaction)) {
        return false;
    }
    child_state = transaction->word16_transaction.state;
    if (transaction->state == TI_C54X_LONG32_PAIR_STATE_IDLE) {
        return child_state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE &&
               transaction->staged_bus_status == TI_C54X_BUS_OK &&
               transaction->request.operand.width ==
                   TI_C54X_DATA_OPERAND_WIDTH_INVALID &&
               transaction->request.operand.first_word == UINT16_C(0) &&
               transaction->request.operand.second_word == UINT16_C(0) &&
               transaction->request.access ==
                   TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID &&
               transaction->request.first_value_msw == UINT16_C(0) &&
               transaction->request.second_value_lsw == UINT16_C(0) &&
               transaction->request.first_transaction_id == UINT64_C(0) &&
               transaction->request.second_transaction_id == UINT64_C(0) &&
               transaction->staged_first_msw == UINT16_C(0) &&
               transaction->staged_second_lsw == UINT16_C(0);
    }
    if (!ti_c54x_long32_pair_request_is_valid(&transaction->request)) {
        return false;
    }
    if (transaction->state == TI_C54X_LONG32_PAIR_STATE_FIRST ||
        transaction->state == TI_C54X_LONG32_PAIR_STATE_SECOND) {
        const bool second =
            transaction->state == TI_C54X_LONG32_PAIR_STATE_SECOND;

        if (transaction->staged_bus_status != TI_C54X_BUS_OK ||
            (transaction->request.access ==
                 TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
             (transaction->staged_first_msw != UINT16_C(0) ||
              transaction->staged_second_lsw != UINT16_C(0))) ||
            (!second &&
             (transaction->staged_first_msw != UINT16_C(0) ||
              transaction->staged_second_lsw != UINT16_C(0))) ||
            (second && transaction->staged_second_lsw != UINT16_C(0)) ||
            !ti_c54x_long32_pair_active_child_is_canonical(
                transaction, second)) {
            return false;
        }
        return true;
    }
    if (transaction->state ==
            TI_C54X_LONG32_PAIR_STATE_COMPLETION_READY ||
        transaction->state == TI_C54X_LONG32_PAIR_STATE_COMPLETE) {
        return transaction->staged_bus_status == TI_C54X_BUS_OK &&
               child_state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE &&
               ti_c54x_long32_pair_child_matches_slot(transaction, true) &&
               (transaction->request.access !=
                    TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE ||
                (transaction->staged_first_msw == UINT16_C(0) &&
                 transaction->staged_second_lsw == UINT16_C(0)));
    }
    return child_state == TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT &&
           ti_c54x_bus_word16_transaction_fault_status_is_valid(
               transaction->staged_bus_status) &&
           transaction->staged_first_msw == UINT16_C(0) &&
           transaction->staged_second_lsw == UINT16_C(0) &&
           (ti_c54x_long32_pair_child_matches_slot(transaction, false) ||
            ti_c54x_long32_pair_child_matches_slot(transaction, true));
}
