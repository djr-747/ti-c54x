/*
 * Module contract
 *
 * Owns: executable contracts for the private fixed-size dual WORD16 READ
 * reducer, including request copying, X-before-Y issue order, resumable
 * WAIT, fault containment, stale identity, and lifecycle boundaries.
 * Does not own: endpoint callbacks, physical memory, arbitration, timing,
 * CPU state, instruction effects, or public ABI policy.
 */

#include "sequencer/instruction/dual_word16_read_transaction.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_dual_word16_read_request read_request(
    uint32_t x_address,
    uint32_t y_address,
    uint64_t x_id,
    uint64_t y_id)
{
    ti_c54x_dual_word16_read_request request = {0};
    const ti_c54x_bus_word16_request_result x =
        ti_c54x_bus_word16_request_read(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, x_address);
    const ti_c54x_bus_word16_request_result y =
        ti_c54x_bus_word16_request_read(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, y_address);

    request.x_request = x.request;
    request.y_request = y.request;
    request.x_transaction_id = x_id;
    request.y_transaction_id = y_id;
    return request;
}

static ti_c54x_bus_word16_observation observation(
    uint64_t id,
    ti_c54x_bus_word16_observation_kind kind,
    uint16_t value,
    ti_c54x_bus_status bus_status)
{
    return (ti_c54x_bus_word16_observation){
        .transaction_id = id,
        .kind = kind,
        .read_value_word = value,
        .bus_status = bus_status};
}

static ti_c54x_dual_word16_read_transaction prepared_transaction(void)
{
    ti_c54x_dual_word16_read_transaction transaction = {0};
    const ti_c54x_dual_word16_read_request request =
        read_request(UINT32_C(0x0100), UINT32_C(0x0abc), UINT64_C(41),
                     UINT64_C(42));

    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_prepare(
                           &transaction, &request) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    return transaction;
}

static void test_request_shape_and_copy(void)
{
    ti_c54x_dual_word16_read_request request =
        read_request(UINT32_C(0x0100), UINT32_C(0x0101), UINT64_C(9),
                     UINT64_C(10));
    ti_c54x_dual_word16_read_request malformed = request;
    ti_c54x_dual_word16_read_transaction transaction = {0};
    ti_c54x_dual_word16_read_transaction snapshot;

    TI_C54X_TEST_CHECK(!ti_c54x_dual_word16_read_request_is_valid(NULL));
    malformed.x_request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    TI_C54X_TEST_CHECK(!ti_c54x_dual_word16_read_request_is_valid(&malformed));
    malformed = request;
    malformed.x_transaction_id = UINT64_C(0);
    TI_C54X_TEST_CHECK(!ti_c54x_dual_word16_read_request_is_valid(&malformed));
    malformed = request;
    malformed.y_transaction_id = malformed.x_transaction_id;
    TI_C54X_TEST_CHECK(!ti_c54x_dual_word16_read_request_is_valid(&malformed));
    malformed = request;
    malformed.y_transaction_id = UINT64_C(8);
    TI_C54X_TEST_CHECK(!ti_c54x_dual_word16_read_request_is_valid(&malformed));
    malformed = request;
    malformed.y_request.write_value_word = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_dual_word16_read_request_is_valid(&malformed));
    malformed = request;
    malformed.y_request.space = TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID;
    TI_C54X_TEST_CHECK(!ti_c54x_dual_word16_read_request_is_valid(&malformed));
    malformed = request;
    malformed.x_request.space = TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM;
    TI_C54X_TEST_CHECK(!ti_c54x_dual_word16_read_request_is_valid(&malformed));
    malformed = request;
    malformed.y_request.space = TI_C54X_BUS_WORD16_REQUEST_SPACE_IO;
    TI_C54X_TEST_CHECK(!ti_c54x_dual_word16_read_request_is_valid(&malformed));
    TI_C54X_TEST_CHECK(
        ti_c54x_dual_word16_read_request_is_valid(&request));
    TI_C54X_TEST_CHECK(sizeof(request) <= 64U);

    snapshot = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_prepare(
                           &transaction, &malformed) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_prepare(
                           &transaction, &request) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    request.x_request.address_word = UINT32_C(0x7777);
    request.x_transaction_id = UINT64_C(99);
    TI_C54X_TEST_CHECK(transaction.request.x_request.address_word ==
                       UINT32_C(0x0100));
    TI_C54X_TEST_CHECK(transaction.request.x_transaction_id == UINT64_C(9));
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_DUAL_WORD16_READ_STATE_X);
    TI_C54X_TEST_CHECK(sizeof(transaction) <= 256U);
}

static void test_wait_order_and_pair_publication(void)
{
    ti_c54x_dual_word16_read_transaction transaction =
        prepared_transaction();
    ti_c54x_dual_word16_read_issue_result issue;
    ti_c54x_dual_word16_read_completion_result completion;
    ti_c54x_bus_word16_transaction child_snapshot;
    const ti_c54x_bus_word16_observation x_wait = observation(
        UINT64_C(41), TI_C54X_BUS_WORD16_OBSERVATION_WAIT, UINT16_C(0),
        TI_C54X_BUS_OK);
    const ti_c54x_bus_word16_observation x_ready = observation(
        UINT64_C(41), TI_C54X_BUS_WORD16_OBSERVATION_READY,
        UINT16_C(0x1234), TI_C54X_BUS_OK);
    const ti_c54x_bus_word16_observation y_wait = observation(
        UINT64_C(42), TI_C54X_BUS_WORD16_OBSERVATION_WAIT, UINT16_C(0),
        TI_C54X_BUS_OK);
    const ti_c54x_bus_word16_observation y_ready = observation(
        UINT64_C(42), TI_C54X_BUS_WORD16_OBSERVATION_READY,
        UINT16_C(0x5678), TI_C54X_BUS_OK);

    issue = ti_c54x_dual_word16_read_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.status == TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(41));
    TI_C54X_TEST_CHECK(issue.request.address_word == UINT32_C(0x0100));
    TI_C54X_TEST_CHECK(issue.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_begin(&transaction)
                           .status ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_poll(
                           &transaction, &x_wait) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_WAIT);
    child_snapshot = transaction.word16_transaction;
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_poll(
                           &transaction, &x_wait) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_WAIT);
    TI_C54X_TEST_CHECK(memcmp(&transaction.word16_transaction,
                              &child_snapshot, sizeof(child_snapshot)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_poll(
                           &transaction, &x_ready) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_DUAL_WORD16_READ_STATE_Y);

    issue = ti_c54x_dual_word16_read_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.status == TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(42));
    TI_C54X_TEST_CHECK(issue.request.address_word == UINT32_C(0x0abc));
    completion = ti_c54x_dual_word16_read_transaction_complete(&transaction);
    TI_C54X_TEST_CHECK(completion.status ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_poll(
                           &transaction, &y_wait) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_poll(
                           &transaction, &y_ready) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_DUAL_WORD16_READ_STATE_COMPLETION_READY);
    completion = ti_c54x_dual_word16_read_transaction_complete(&transaction);
    TI_C54X_TEST_CHECK(completion.status ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(completion.x_value_word == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(completion.y_value_word == UINT16_C(0x5678));
    TI_C54X_TEST_CHECK(completion.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_DUAL_WORD16_READ_STATE_COMPLETE);
}

static void test_stale_fault_cancel_abandon_and_reset_boundaries(void)
{
    ti_c54x_dual_word16_read_transaction transaction =
        prepared_transaction();
    ti_c54x_dual_word16_read_transaction snapshot;
    ti_c54x_dual_word16_read_completion_result completion;
    const ti_c54x_bus_word16_observation stale = observation(
        UINT64_C(40), TI_C54X_BUS_WORD16_OBSERVATION_READY,
        UINT16_C(0xffff), TI_C54X_BUS_OK);
    const ti_c54x_bus_word16_observation x_fault = observation(
        UINT64_C(41), TI_C54X_BUS_WORD16_OBSERVATION_FAULT, UINT16_C(0),
        TI_C54X_BUS_FAULT);
    const ti_c54x_bus_word16_observation x_ready = observation(
        UINT64_C(41), TI_C54X_BUS_WORD16_OBSERVATION_READY,
        UINT16_C(0x1111), TI_C54X_BUS_OK);
    const ti_c54x_bus_word16_observation y_fault = observation(
        UINT64_C(42), TI_C54X_BUS_WORD16_OBSERVATION_FAULT, UINT16_C(0),
        TI_C54X_BUS_UNMAPPED);

    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_begin(&transaction)
                           .status == TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    snapshot = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_poll(
                           &transaction, &stale) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_STALE_TRANSACTION);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_cancel(
                           &transaction, UINT64_C(40)) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_STALE_TRANSACTION);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_cancel(
                           &transaction, UINT64_C(41)) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_DUAL_WORD16_READ_STATE_IDLE);

    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_prepare(
                           &transaction, &(ti_c54x_dual_word16_read_request){
                               .x_request = {TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                             TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                                             UINT32_C(0x0100), UINT16_C(0)},
                               .y_request = {TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
                                             TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
                                             UINT32_C(0x0abc), UINT16_C(0)},
                               .x_transaction_id = UINT64_C(41),
                               .y_transaction_id = UINT64_C(42)}) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_begin(&transaction)
                           .status == TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_poll(
                           &transaction, &x_fault) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_FAULT);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_DUAL_WORD16_READ_STATE_FAULT);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_begin(&transaction)
                           .status ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE);
    completion = ti_c54x_dual_word16_read_transaction_complete(&transaction);
    TI_C54X_TEST_CHECK(completion.status ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_FAULT);
    TI_C54X_TEST_CHECK(completion.x_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(completion.y_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_reset(&transaction)
                           == TI_C54X_DUAL_WORD16_READ_STATUS_OK);

    transaction = prepared_transaction();
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_begin(&transaction)
                           .status == TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_poll(
                           &transaction, &x_ready) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_abandon_staged(
                           &transaction) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_DUAL_WORD16_READ_STATE_IDLE);

    transaction = prepared_transaction();
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_begin(&transaction)
                           .status == TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_poll(
                           &transaction, &x_ready) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_begin(&transaction)
                           .status == TI_C54X_DUAL_WORD16_READ_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_poll(
                           &transaction, &y_fault) ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_FAULT);
    completion = ti_c54x_dual_word16_read_transaction_complete(&transaction);
    TI_C54X_TEST_CHECK(completion.status ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_FAULT);
    TI_C54X_TEST_CHECK(completion.x_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(completion.y_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(completion.bus_status == TI_C54X_BUS_UNMAPPED);
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_reset(&transaction)
                           == TI_C54X_DUAL_WORD16_READ_STATUS_OK);

    transaction = prepared_transaction();
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_reset(&transaction)
                           == TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE);
}

static void test_malformed_active_storage_is_rejected(void)
{
    ti_c54x_dual_word16_read_transaction transaction =
        prepared_transaction();
    ti_c54x_dual_word16_read_transaction snapshot;

    transaction.word16_transaction.transaction_id = UINT64_C(99);
    snapshot = transaction;
    TI_C54X_TEST_CHECK(!ti_c54x_dual_word16_read_transaction_is_canonical(
        &transaction));
    TI_C54X_TEST_CHECK(ti_c54x_dual_word16_read_transaction_begin(&transaction)
                           .status ==
                       TI_C54X_DUAL_WORD16_READ_STATUS_INVALID_STATE);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
}

int main(void)
{
    test_request_shape_and_copy();
    test_wait_order_and_pair_publication();
    test_stale_fault_cancel_abandon_and_reset_boundaries();
    test_malformed_active_storage_is_rejected();
    puts("dual_word16_read_transaction: ok");
    return 0;
}
