/*
 * Module contract
 *
 * Owns: executable LONG32 pair READ lifecycle contracts for the private
 * sequencer reducer: exact request order, copied transaction IDs, WAIT,
 * stale observations, faults, cancellation, and final publication.
 * Does not own: physical endpoint arbitration, CPU state, opcode decode,
 * address planning, timing, or write visibility.
 */

#include "sequencer/instruction/long32_pair_transaction.h"
#include "address/data_operand.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_long32_pair_request
read_request(void)
{
    ti_c54x_long32_pair_request request = {0};

    request.operand = ti_c54x_data_operand_long32(UINT16_C(0x0400));
    request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    request.first_transaction_id = UINT64_C(7001);
    request.second_transaction_id = UINT64_C(7002);
    return request;
}

static ti_c54x_bus_word16_observation
observation(uint64_t transaction_id,
            ti_c54x_bus_word16_observation_kind kind,
            uint16_t value,
            ti_c54x_bus_status bus_status)
{
    return (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = kind,
        .read_value_word = value,
        .bus_status = bus_status};
}

static ti_c54x_long32_pair_transaction
prepared_transaction(void)
{
    ti_c54x_long32_pair_transaction transaction = {0};
    const ti_c54x_long32_pair_request request = read_request();

    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_prepare(
                           &transaction, &request) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    return transaction;
}

static void
test_wait_and_exact_two_word_order(void)
{
    ti_c54x_long32_pair_transaction transaction = prepared_transaction();
    ti_c54x_bus_word16_transaction child_snapshot;
    ti_c54x_long32_pair_issue_result issue;
    ti_c54x_long32_pair_completion_result completion;
    const ti_c54x_bus_word16_observation first_wait = observation(
        UINT64_C(7001), TI_C54X_BUS_WORD16_OBSERVATION_WAIT, UINT16_C(0),
        TI_C54X_BUS_OK);
    const ti_c54x_bus_word16_observation first_ready = observation(
        UINT64_C(7001), TI_C54X_BUS_WORD16_OBSERVATION_READY,
        UINT16_C(0x1234), TI_C54X_BUS_OK);
    const ti_c54x_bus_word16_observation second_wait = observation(
        UINT64_C(7002), TI_C54X_BUS_WORD16_OBSERVATION_WAIT, UINT16_C(0),
        TI_C54X_BUS_OK);
    const ti_c54x_bus_word16_observation second_ready = observation(
        UINT64_C(7002), TI_C54X_BUS_WORD16_OBSERVATION_READY,
        UINT16_C(0x5678), TI_C54X_BUS_OK);

    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.status == TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(7001));
    TI_C54X_TEST_CHECK(issue.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(issue.request.address_word == UINT32_C(0x0400));
    {
        const ti_c54x_long32_pair_transaction snapshot = transaction;

        issue = ti_c54x_long32_pair_transaction_begin(&transaction);
        TI_C54X_TEST_CHECK(issue.status ==
                           TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
        TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                           0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &first_wait) ==
                       TI_C54X_LONG32_PAIR_STATUS_WAIT);
    TI_C54X_TEST_CHECK(transaction.state == TI_C54X_LONG32_PAIR_STATE_FIRST);
    child_snapshot = transaction.word16_transaction;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &first_wait) ==
                       TI_C54X_LONG32_PAIR_STATUS_WAIT);
    TI_C54X_TEST_CHECK(memcmp(&transaction.word16_transaction,
                              &child_snapshot,
                              sizeof(child_snapshot)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &first_ready) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.state == TI_C54X_LONG32_PAIR_STATE_SECOND);

    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.status == TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(7002));
    TI_C54X_TEST_CHECK(issue.request.address_word == UINT32_C(0x0401));
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &second_wait) ==
                       TI_C54X_LONG32_PAIR_STATUS_WAIT);
    child_snapshot = transaction.word16_transaction;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &second_wait) ==
                       TI_C54X_LONG32_PAIR_STATUS_WAIT);
    TI_C54X_TEST_CHECK(memcmp(&transaction.word16_transaction,
                              &child_snapshot,
                              sizeof(child_snapshot)) == 0);
    completion = ti_c54x_long32_pair_transaction_complete(&transaction);
    TI_C54X_TEST_CHECK(completion.status == TI_C54X_LONG32_PAIR_STATUS_WAIT);
    TI_C54X_TEST_CHECK(transaction.state == TI_C54X_LONG32_PAIR_STATE_SECOND);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &second_ready) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_LONG32_PAIR_STATE_COMPLETION_READY);
    completion = ti_c54x_long32_pair_transaction_complete(&transaction);
    TI_C54X_TEST_CHECK(completion.status == TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(completion.first_msw == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(completion.second_lsw == UINT16_C(0x5678));
    TI_C54X_TEST_CHECK(completion.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(transaction.state == TI_C54X_LONG32_PAIR_STATE_COMPLETE);
    {
        const ti_c54x_long32_pair_transaction snapshot = transaction;

        TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                               &transaction, &second_ready) ==
                           TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
        TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_cancel(
                               &transaction, UINT64_C(7002)) ==
                           TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
        TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot,
                                  sizeof(transaction)) == 0);
    }
}

static void
test_stale_and_cancel_are_transactional(void)
{
    ti_c54x_long32_pair_transaction transaction = prepared_transaction();
    ti_c54x_long32_pair_transaction snapshot;
    ti_c54x_long32_pair_issue_result issue;
    const ti_c54x_bus_word16_observation stale = observation(
        UINT64_C(6999), TI_C54X_BUS_WORD16_OBSERVATION_READY,
        UINT16_C(0xaaaa), TI_C54X_BUS_OK);

    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(7001));
    snapshot = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &stale) ==
                       TI_C54X_LONG32_PAIR_STATUS_STALE_TRANSACTION);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
    snapshot = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_cancel(
                           &transaction, UINT64_C(0)) ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
    snapshot = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_cancel(
                           &transaction, UINT64_C(7000)) ==
                       TI_C54X_LONG32_PAIR_STATUS_STALE_TRANSACTION);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_cancel(
                           &transaction, UINT64_C(7001)) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.state == TI_C54X_LONG32_PAIR_STATE_IDLE);
    snapshot = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &stale) ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_complete(&transaction)
                           .status ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_reset(&transaction) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
}

static void
test_first_and_second_faults_publish_nothing(void)
{
    ti_c54x_long32_pair_transaction transaction = prepared_transaction();
    ti_c54x_long32_pair_issue_result issue;
    ti_c54x_long32_pair_completion_result completion;
    const ti_c54x_bus_word16_observation first_fault = observation(
        UINT64_C(7001), TI_C54X_BUS_WORD16_OBSERVATION_FAULT, UINT16_C(0),
        TI_C54X_BUS_FAULT);
    const ti_c54x_bus_word16_observation first_ready = observation(
        UINT64_C(7001), TI_C54X_BUS_WORD16_OBSERVATION_READY,
        UINT16_C(0x1111), TI_C54X_BUS_OK);
    const ti_c54x_bus_word16_observation second_fault = observation(
        UINT64_C(7002), TI_C54X_BUS_WORD16_OBSERVATION_FAULT, UINT16_C(0),
        TI_C54X_BUS_FAULT);

    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(7001));
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &first_fault) ==
                       TI_C54X_LONG32_PAIR_STATUS_FAULT);
    TI_C54X_TEST_CHECK(transaction.state == TI_C54X_LONG32_PAIR_STATE_FAULT);
    {
        const ti_c54x_long32_pair_transaction snapshot = transaction;

        issue = ti_c54x_long32_pair_transaction_begin(&transaction);
        TI_C54X_TEST_CHECK(issue.status ==
                           TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
        TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                               &transaction, &first_fault) ==
                           TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
        TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_cancel(
                               &transaction, UINT64_C(7001)) ==
                           TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
        TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot,
                                  sizeof(transaction)) == 0);
    }
    completion = ti_c54x_long32_pair_transaction_complete(&transaction);
    TI_C54X_TEST_CHECK(completion.status == TI_C54X_LONG32_PAIR_STATUS_FAULT);
    TI_C54X_TEST_CHECK(completion.first_msw == UINT16_C(0));
    TI_C54X_TEST_CHECK(completion.second_lsw == UINT16_C(0));

    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_reset(&transaction) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    {
        ti_c54x_long32_pair_request request = read_request();
        TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_prepare(
                               &transaction, &request) ==
                           TI_C54X_LONG32_PAIR_STATUS_OK);
    }
    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(7001));
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &first_ready) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(7002));
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &second_fault) ==
                       TI_C54X_LONG32_PAIR_STATUS_FAULT);
    completion = ti_c54x_long32_pair_transaction_complete(&transaction);
    TI_C54X_TEST_CHECK(completion.status == TI_C54X_LONG32_PAIR_STATUS_FAULT);
    TI_C54X_TEST_CHECK(completion.first_msw == UINT16_C(0));
    TI_C54X_TEST_CHECK(completion.second_lsw == UINT16_C(0));
}

int
main(void)
{
    test_wait_and_exact_two_word_order();
    test_stale_and_cancel_are_transactional();
    test_first_and_second_faults_publish_nothing();
    puts("long32_pair_read_transaction: ok");
    return 0;
}
