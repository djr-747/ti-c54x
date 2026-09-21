/*
 * Module contract
 *
 * Owns: one fixed-size, allocation-free reducer for two DATA
 * WORD16 READs plus one PROGRAM WORD16 READ. It runs the
 * shared dual reducer for the X/Y DATA legs, then one single
 * WORD16 transaction for the PROGRAM leg, staging each value
 * before the next leg issues and publishing all three only at
 * triple completion. Fresh nonzero strictly increasing IDs
 * are required across X, Y, and program legs.
 * Does not own: endpoint callbacks, physical memory or visibility, bus
 * arbitration, timing, CPU state, opcode or effect publication, observers,
 * diagnostics, environment, globals, heap, or I/O. Request and observation
 * pointers are borrowed only for the duration of a call; accepted values are
 * copied into caller-owned transaction storage.
 */

#include "triple_word16_read_transaction.h"

#include <stddef.h>
#include <stdint.h>

#include <ti_c54x/bus_endpoint.h>

#include "bus/transaction/word16_transaction.h"
#include "bus/transaction/word16_transaction_validate.h"

static bool state_is_valid(ti_c54x_triple_word16_read_state state)
{
    return state == TI_C54X_TRIPLE_WORD16_READ_STATE_IDLE ||
           state == TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL ||
           state == TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM ||
           state == TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETION_READY ||
           state == TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETE ||
           state == TI_C54X_TRIPLE_WORD16_READ_STATE_FAULT;
}

static bool program_request_is_valid(
    const ti_c54x_bus_word16_request *request)
{
    return request != NULL &&
           ti_c54x_bus_word16_request_is_valid(request) &&
           request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM &&
           request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
           request->write_value_word == UINT16_C(0);
}

bool ti_c54x_triple_word16_read_request_is_valid(
    const ti_c54x_triple_word16_read_request *request)
{
    ti_c54x_dual_word16_read_request dual_request;

    if (request == NULL ||
        !program_request_is_valid(&request->program_request) ||
        request->x_transaction_id == UINT64_C(0) ||
        request->y_transaction_id == UINT64_C(0) ||
        request->program_transaction_id == UINT64_C(0) ||
        request->y_transaction_id <= request->x_transaction_id ||
        request->program_transaction_id <= request->y_transaction_id) {
        return false;
    }
    dual_request.x_request = request->x_request;
    dual_request.y_request = request->y_request;
    dual_request.x_transaction_id = request->x_transaction_id;
    dual_request.y_transaction_id = request->y_transaction_id;
    return ti_c54x_dual_word16_read_request_is_valid(&dual_request);
}

bool ti_c54x_triple_word16_read_transaction_is_canonical(
    const ti_c54x_triple_word16_read_transaction *transaction)
{
    if (transaction == NULL || !state_is_valid(transaction->state) ||
        !ti_c54x_dual_word16_read_transaction_is_canonical(
            &transaction->dual) ||
        !ti_c54x_bus_word16_transaction_is_canonical(
            &transaction->program)) {
        return false;
    }
    if (transaction->request.x_transaction_id == UINT64_C(0) ||
        transaction->request.y_transaction_id == UINT64_C(0) ||
        transaction->request.program_transaction_id == UINT64_C(0) ||
        transaction->request.y_transaction_id <=
            transaction->request.x_transaction_id ||
        transaction->request.program_transaction_id <=
            transaction->request.y_transaction_id) {
        return transaction->state ==
                   TI_C54X_TRIPLE_WORD16_READ_STATE_IDLE &&
               transaction->request.x_request.address_word == UINT32_C(0) &&
               transaction->request.y_request.address_word == UINT32_C(0) &&
               transaction->request.program_request.address_word ==
                   UINT32_C(0) &&
               transaction->staged_x_value_word == UINT16_C(0) &&
               transaction->staged_y_value_word == UINT16_C(0) &&
               transaction->staged_program_value_word == UINT16_C(0) &&
               transaction->staged_bus_status == TI_C54X_BUS_OK;
    }
    if (transaction->state == TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL) {
        return transaction->staged_bus_status == TI_C54X_BUS_OK &&
               transaction->staged_x_value_word == UINT16_C(0) &&
               transaction->staged_y_value_word == UINT16_C(0) &&
               transaction->staged_program_value_word == UINT16_C(0);
    }
    if (transaction->state == TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM) {
        return transaction->staged_bus_status == TI_C54X_BUS_OK &&
               transaction->staged_program_value_word == UINT16_C(0);
    }
    if (transaction->state ==
            TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETION_READY ||
        transaction->state == TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETE) {
        return transaction->staged_bus_status == TI_C54X_BUS_OK;
    }
    return ti_c54x_bus_word16_transaction_fault_status_is_valid(
               transaction->staged_bus_status) &&
           transaction->staged_x_value_word == UINT16_C(0) &&
           transaction->staged_y_value_word == UINT16_C(0) &&
           transaction->staged_program_value_word == UINT16_C(0);
}

ti_c54x_triple_word16_read_status
ti_c54x_triple_word16_read_transaction_prepare(
    ti_c54x_triple_word16_read_transaction *transaction,
    const ti_c54x_triple_word16_read_request *request)
{
    ti_c54x_dual_word16_read_request dual_request;

    if (transaction == NULL || request == NULL) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_triple_word16_read_transaction_is_canonical(transaction) ||
        transaction->state != TI_C54X_TRIPLE_WORD16_READ_STATE_IDLE) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
    }
    if (!ti_c54x_triple_word16_read_request_is_valid(request)) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    dual_request.x_request = request->x_request;
    dual_request.y_request = request->y_request;
    dual_request.x_transaction_id = request->x_transaction_id;
    dual_request.y_transaction_id = request->y_transaction_id;
    if (ti_c54x_dual_word16_read_transaction_prepare(&transaction->dual,
                                                     &dual_request) !=
        TI_C54X_DUAL_WORD16_READ_STATUS_OK) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
    }
    transaction->request.x_request.address_word =
        request->x_request.address_word;
    transaction->request.y_request.address_word =
        request->y_request.address_word;
    transaction->request.program_request.address_word =
        request->program_request.address_word;
    transaction->request.x_transaction_id = request->x_transaction_id;
    transaction->request.y_transaction_id = request->y_transaction_id;
    transaction->request.program_transaction_id =
        request->program_transaction_id;
    transaction->state = TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL;
    return TI_C54X_TRIPLE_WORD16_READ_STATUS_OK;
}

ti_c54x_triple_word16_read_issue_result
ti_c54x_triple_word16_read_transaction_begin(
    ti_c54x_triple_word16_read_transaction *transaction)
{
    ti_c54x_triple_word16_read_issue_result result = {
        TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE, UINT64_C(0),
        {0}};

    if (transaction == NULL ||
        !ti_c54x_triple_word16_read_transaction_is_canonical(transaction)) {
        result.status = TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_ARGUMENT;
        return result;
    }
    if (transaction->state == TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL) {
        ti_c54x_dual_word16_read_issue_result dual_issue =
            ti_c54x_dual_word16_read_transaction_begin(&transaction->dual);

        result.transaction_id = dual_issue.transaction_id;
        result.request = dual_issue.request;
        result.status = dual_issue.status ==
                                TI_C54X_DUAL_WORD16_READ_STATUS_OK
                            ? TI_C54X_TRIPLE_WORD16_READ_STATUS_OK
                            : TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
        return result;
    }
    if (transaction->state == TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM) {
        ti_c54x_bus_word16_transaction_issue_result program_issue =
            ti_c54x_bus_word16_transaction_begin(&transaction->program);

        result.transaction_id = program_issue.transaction_id;
        result.request = program_issue.request;
        result.status = program_issue.status ==
                                TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK
                            ? TI_C54X_TRIPLE_WORD16_READ_STATUS_OK
                            : TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
        return result;
    }
    return result;
}

static ti_c54x_triple_word16_read_status reduce_dual_ready(
    ti_c54x_triple_word16_read_transaction *transaction)
{
    ti_c54x_dual_word16_read_completion_result dual_completion =
        ti_c54x_dual_word16_read_transaction_complete(&transaction->dual);
    ti_c54x_bus_word16_request program_request = {0};

    if (dual_completion.status != TI_C54X_DUAL_WORD16_READ_STATUS_OK) {
        transaction->state = TI_C54X_TRIPLE_WORD16_READ_STATE_FAULT;
        transaction->staged_bus_status = dual_completion.bus_status;
        transaction->staged_x_value_word = UINT16_C(0);
        transaction->staged_y_value_word = UINT16_C(0);
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_FAULT;
    }
    transaction->staged_x_value_word = dual_completion.x_value_word;
    transaction->staged_y_value_word = dual_completion.y_value_word;
    program_request.space = TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM;
    program_request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    program_request.address_word =
        transaction->request.program_request.address_word;
    program_request.write_value_word = UINT16_C(0);
    if (ti_c54x_bus_word16_transaction_prepare(
            &transaction->program, &program_request,
            transaction->request.program_transaction_id) !=
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
        transaction->state = TI_C54X_TRIPLE_WORD16_READ_STATE_FAULT;
        transaction->staged_x_value_word = UINT16_C(0);
        transaction->staged_y_value_word = UINT16_C(0);
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_FAULT;
    }
    transaction->state = TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM;
    /* The pair is staged and the PROGRAM leg is prepared; the caller
     * issues it next. */
    return TI_C54X_TRIPLE_WORD16_READ_STATUS_OK;
}

ti_c54x_triple_word16_read_status ti_c54x_triple_word16_read_transaction_poll(
    ti_c54x_triple_word16_read_transaction *transaction,
    const ti_c54x_bus_word16_observation *observation)
{
    if (transaction == NULL || observation == NULL) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (!ti_c54x_triple_word16_read_transaction_is_canonical(transaction)) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
    }
    if (transaction->state == TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL) {
        ti_c54x_dual_word16_read_status dual_status =
            ti_c54x_dual_word16_read_transaction_poll(&transaction->dual,
                                                      observation);

        if (dual_status == TI_C54X_DUAL_WORD16_READ_STATUS_WAIT) {
            return TI_C54X_TRIPLE_WORD16_READ_STATUS_WAIT;
        }
        if (dual_status == TI_C54X_DUAL_WORD16_READ_STATUS_FAULT) {
            transaction->state = TI_C54X_TRIPLE_WORD16_READ_STATE_FAULT;
            transaction->staged_bus_status =
                transaction->dual.staged_bus_status;
            transaction->staged_x_value_word = UINT16_C(0);
            transaction->staged_y_value_word = UINT16_C(0);
            return TI_C54X_TRIPLE_WORD16_READ_STATUS_FAULT;
        }
        if (dual_status != TI_C54X_DUAL_WORD16_READ_STATUS_OK) {
            return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
        }
        /* Dual OK after the X leg means the Y leg is prepared, not
         * that the pair is complete; the caller issues the Y leg
         * next. Only reduce once the dual member stages both
         * values. */
        if (transaction->dual.state !=
            TI_C54X_DUAL_WORD16_READ_STATE_COMPLETION_READY) {
            return TI_C54X_TRIPLE_WORD16_READ_STATUS_OK;
        }
        return reduce_dual_ready(transaction);
    }
    if (transaction->state == TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM) {
        ti_c54x_bus_word16_transaction_status program_status =
            ti_c54x_bus_word16_transaction_poll(&transaction->program,
                                                observation);
        ti_c54x_bus_word16_transaction_completion_result completion;

        if (program_status == TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT) {
            return TI_C54X_TRIPLE_WORD16_READ_STATUS_WAIT;
        }
        if (program_status == TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT) {
            transaction->state = TI_C54X_TRIPLE_WORD16_READ_STATE_FAULT;
            transaction->staged_bus_status =
                transaction->program.staged_bus_status;
            transaction->staged_x_value_word = UINT16_C(0);
            transaction->staged_y_value_word = UINT16_C(0);
            return TI_C54X_TRIPLE_WORD16_READ_STATUS_FAULT;
        }
        if (program_status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
        }
        completion = ti_c54x_bus_word16_transaction_complete(
            &transaction->program,
            transaction->request.program_transaction_id);
        if (completion.status != TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            transaction->state = TI_C54X_TRIPLE_WORD16_READ_STATE_FAULT;
            transaction->staged_bus_status = completion.bus_status;
            transaction->staged_x_value_word = UINT16_C(0);
            transaction->staged_y_value_word = UINT16_C(0);
            return TI_C54X_TRIPLE_WORD16_READ_STATUS_FAULT;
        }
        transaction->staged_program_value_word = completion.read_value_word;
        transaction->state =
            TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETION_READY;
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_OK;
    }
    return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
}

ti_c54x_triple_word16_read_status ti_c54x_triple_word16_read_transaction_cancel(
    ti_c54x_triple_word16_read_transaction *transaction,
    uint64_t transaction_id)
{
    bool cancelled = false;

    if (transaction == NULL || transaction_id == UINT64_C(0) ||
        !ti_c54x_triple_word16_read_transaction_is_canonical(transaction)) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->state == TI_C54X_TRIPLE_WORD16_READ_STATE_DUAL) {
        if (ti_c54x_dual_word16_read_transaction_cancel(&transaction->dual,
                                                        transaction_id) !=
            TI_C54X_DUAL_WORD16_READ_STATUS_OK) {
            return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
        }
        cancelled = true;
    } else if (transaction->state ==
                   TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM &&
               transaction_id == transaction->request.program_transaction_id) {
        if (ti_c54x_bus_word16_transaction_cancel(&transaction->program,
                                                  transaction_id) !=
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK) {
            return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
        }
        cancelled = true;
    }
    if (!cancelled) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
    }
    *transaction = (ti_c54x_triple_word16_read_transaction){0};
    return TI_C54X_TRIPLE_WORD16_READ_STATUS_OK;
}

ti_c54x_triple_word16_read_status
ti_c54x_triple_word16_read_transaction_abandon_staged(
    ti_c54x_triple_word16_read_transaction *transaction)
{
    if (transaction == NULL ||
        !ti_c54x_triple_word16_read_transaction_is_canonical(transaction)) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->state !=
            TI_C54X_TRIPLE_WORD16_READ_STATE_PROGRAM &&
        transaction->state !=
            TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETION_READY) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
    }
    *transaction = (ti_c54x_triple_word16_read_transaction){0};
    return TI_C54X_TRIPLE_WORD16_READ_STATUS_OK;
}

ti_c54x_triple_word16_read_completion_result
ti_c54x_triple_word16_read_transaction_complete(
    ti_c54x_triple_word16_read_transaction *transaction)
{
    ti_c54x_triple_word16_read_completion_result result = {
        TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE, UINT16_C(0),
        UINT16_C(0), UINT16_C(0), TI_C54X_BUS_OK};

    if (transaction == NULL ||
        !ti_c54x_triple_word16_read_transaction_is_canonical(transaction) ||
        transaction->state !=
            TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETION_READY) {
        result.status = transaction == NULL
                            ? TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_ARGUMENT
                            : TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
        return result;
    }
    result.status = TI_C54X_TRIPLE_WORD16_READ_STATUS_OK;
    result.x_value_word = transaction->staged_x_value_word;
    result.y_value_word = transaction->staged_y_value_word;
    result.program_value_word = transaction->staged_program_value_word;
    result.bus_status = TI_C54X_BUS_OK;
    transaction->state = TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETE;
    return result;
}

ti_c54x_triple_word16_read_status ti_c54x_triple_word16_read_transaction_reset(
    ti_c54x_triple_word16_read_transaction *transaction)
{
    if (transaction == NULL ||
        !ti_c54x_triple_word16_read_transaction_is_canonical(transaction)) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_ARGUMENT;
    }
    if (transaction->state != TI_C54X_TRIPLE_WORD16_READ_STATE_IDLE &&
        transaction->state != TI_C54X_TRIPLE_WORD16_READ_STATE_COMPLETE &&
        transaction->state != TI_C54X_TRIPLE_WORD16_READ_STATE_FAULT) {
        return TI_C54X_TRIPLE_WORD16_READ_STATUS_INVALID_STATE;
    }
    *transaction = (ti_c54x_triple_word16_read_transaction){0};
    return TI_C54X_TRIPLE_WORD16_READ_STATUS_OK;
}
