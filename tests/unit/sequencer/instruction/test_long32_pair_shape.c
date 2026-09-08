/*
 * Module contract
 *
 * Owns: RED and GREEN executable contracts for the private LONG32 pair
 * descriptor and reducer shape. It proves canonical parity, copied request
 * ownership, request rejection, ordering, and size guardrails.
 * Does not own: endpoint visibility, CPU retirement, opcode decode, address
 * arithmetic, timing, or public ABI policy.
 */

#include "sequencer/instruction/long32_pair_transaction.h"
#include "address/data_operand.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ti_c54x_long32_pair_request
read_request(uint16_t first_word, uint64_t first_id, uint64_t second_id)
{
    ti_c54x_long32_pair_request request = {0};

    request.operand = ti_c54x_data_operand_long32(first_word);
    request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ;
    request.first_transaction_id = first_id;
    request.second_transaction_id = second_id;
    return request;
}

static ti_c54x_long32_pair_request
write_request(uint16_t first_word,
              uint16_t first_value,
              uint16_t second_value,
              uint64_t first_id,
              uint64_t second_id)
{
    ti_c54x_long32_pair_request request = read_request(first_word, first_id,
                                                       second_id);

    request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    request.first_value_msw = first_value;
    request.second_value_lsw = second_value;
    return request;
}

static void
test_request_shape(void)
{
    ti_c54x_long32_pair_request malformed = {0};
    ti_c54x_long32_pair_request request;

    TI_C54X_TEST_CHECK(!ti_c54x_long32_pair_request_is_valid(NULL));
    TI_C54X_TEST_CHECK(!ti_c54x_long32_pair_request_is_valid(&malformed));

    request = read_request(UINT16_C(0x0100), UINT64_C(11), UINT64_C(12));
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_request_is_valid(&request));
    request = read_request(UINT16_C(0x0101), UINT64_C(21), UINT64_C(22));
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_request_is_valid(&request));

    request.operand = ti_c54x_data_operand_word16(UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(!ti_c54x_long32_pair_request_is_valid(&request));
    request = read_request(UINT16_C(0x0100), UINT64_C(11), UINT64_C(12));
    request.operand.second_word = request.operand.first_word;
    TI_C54X_TEST_CHECK(!ti_c54x_long32_pair_request_is_valid(&request));
    request.operand.second_word = UINT16_C(0x0102);
    TI_C54X_TEST_CHECK(!ti_c54x_long32_pair_request_is_valid(&request));
    request = read_request(UINT16_C(0x0100), UINT64_C(11), UINT64_C(12));
    request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID;
    TI_C54X_TEST_CHECK(!ti_c54x_long32_pair_request_is_valid(&request));

    request = read_request(UINT16_C(0x0100), UINT64_C(11), UINT64_C(12));
    request.first_value_msw = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_long32_pair_request_is_valid(&request));
    request = read_request(UINT16_C(0x0100), UINT64_C(11), UINT64_C(12));
    request.second_value_lsw = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_long32_pair_request_is_valid(&request));

    request = write_request(UINT16_C(0x0100), UINT16_C(0x1234),
                            UINT16_C(0x5678), UINT64_C(0), UINT64_C(2));
    TI_C54X_TEST_CHECK(!ti_c54x_long32_pair_request_is_valid(&request));
    request.first_transaction_id = UINT64_C(2);
    request.second_transaction_id = UINT64_C(2);
    TI_C54X_TEST_CHECK(!ti_c54x_long32_pair_request_is_valid(&request));
    request.second_transaction_id = UINT64_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_long32_pair_request_is_valid(&request));
}

static void
test_prepare_is_transactional_and_copies_request(void)
{
    const ti_c54x_long32_pair_request original =
        write_request(UINT16_C(0x0201), UINT16_C(0x1234), UINT16_C(0x5678),
                      UINT64_C(101), UINT64_C(102));
    ti_c54x_long32_pair_request caller = original;
    ti_c54x_long32_pair_transaction transaction = {0};
    ti_c54x_long32_pair_transaction snapshot;
    ti_c54x_long32_pair_request invalid = original;
    ti_c54x_long32_pair_issue_result issue;

    snapshot = transaction;
    invalid.operand.width = TI_C54X_DATA_OPERAND_WIDTH_WORD16;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_prepare(
                           &transaction, &invalid) ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_prepare(
                           &transaction, &caller) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    caller.first_value_msw = UINT16_C(0);
    caller.second_value_lsw = UINT16_C(0);
    caller.first_transaction_id = UINT64_C(999);
    TI_C54X_TEST_CHECK(transaction.request.first_value_msw ==
                       original.first_value_msw);
    TI_C54X_TEST_CHECK(transaction.request.second_value_lsw ==
                       original.second_value_lsw);
    TI_C54X_TEST_CHECK(transaction.request.first_transaction_id ==
                       original.first_transaction_id);
    TI_C54X_TEST_CHECK(transaction.request.second_transaction_id ==
                       original.second_transaction_id);
    TI_C54X_TEST_CHECK(transaction.state == TI_C54X_LONG32_PAIR_STATE_FIRST);

    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.status == TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(101));
    TI_C54X_TEST_CHECK(issue.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(issue.request.address_word == UINT32_C(0x0201));
    TI_C54X_TEST_CHECK(issue.request.write_value_word == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(transaction.state == TI_C54X_LONG32_PAIR_STATE_FIRST);
}

static void
test_size_and_order_shape(void)
{
    const ti_c54x_long32_pair_request request =
        read_request(UINT16_C(0x0200), UINT64_C(301), UINT64_C(302));
    ti_c54x_long32_pair_transaction transaction = {0};
    ti_c54x_long32_pair_issue_result issue;
    ti_c54x_bus_word16_observation observation = {
        .transaction_id = UINT64_C(301),
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = UINT16_C(0xabcd),
        .bus_status = TI_C54X_BUS_OK};

    (void)printf("long32_pair sizeof(request)=%zu sizeof(transaction)=%zu\n",
                 sizeof(request), sizeof(transaction));
    TI_C54X_TEST_CHECK(sizeof(request) <= 64U);
    TI_C54X_TEST_CHECK(sizeof(transaction) <= 256U);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_prepare(
                           &transaction, &request) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(301));
    TI_C54X_TEST_CHECK(issue.request.address_word == UINT32_C(0x0200));
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(302));
    TI_C54X_TEST_CHECK(issue.request.address_word == UINT32_C(0x0201));
}

static void
test_null_arguments_are_rejected_transactionally(void)
{
    const ti_c54x_long32_pair_request request =
        read_request(UINT16_C(0x0300), UINT64_C(401), UINT64_C(402));
    const ti_c54x_bus_word16_observation observation = {
        .transaction_id = UINT64_C(401),
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = UINT16_C(0xaaaa),
        .bus_status = TI_C54X_BUS_OK};
    ti_c54x_long32_pair_transaction transaction = {0};
    ti_c54x_long32_pair_transaction snapshot;
    ti_c54x_long32_pair_issue_result issue;
    ti_c54x_long32_pair_completion_result completion;

    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_prepare(
                           NULL, &request) ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
    snapshot = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_prepare(
                           &transaction, NULL) ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
    issue = ti_c54x_long32_pair_transaction_begin(NULL);
    TI_C54X_TEST_CHECK(issue.status == TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
    snapshot = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           NULL, &observation) ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, NULL) ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_cancel(NULL,
                                                              UINT64_C(1)) ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
    completion = ti_c54x_long32_pair_transaction_complete(NULL);
    TI_C54X_TEST_CHECK(completion.status ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_reset(NULL) ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_ARGUMENT);
}

int
main(void)
{
    test_request_shape();
    test_prepare_is_transactional_and_copies_request();
    test_size_and_order_shape();
    test_null_arguments_are_rejected_transactionally();
    puts("long32_pair_shape: ok");
    return 0;
}
