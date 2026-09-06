/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY lifecycle proof for the private
 * callback-free WORD16 transaction reducer.
 * Does not own: external issue counts, memory visibility, arbitration, device
 * timing, public ABI, or test discovery.
 */

#include "../../../../src/bus/request/word16_request.h"
#include "../../../../src/bus/transaction/word16_transaction.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

static ti_c54x_bus_word16_request make_request(
    ti_c54x_bus_word16_request_space space,
    ti_c54x_bus_word16_request_access access)
{
    const ti_c54x_bus_word16_request_result result =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
            ? ti_c54x_bus_word16_request_read(space, UINT32_C(0x1234))
            : ti_c54x_bus_word16_request_write(space, UINT32_C(0x1234),
                                               UINT16_C(0xbeef));

    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_REQUEST_STATUS_OK);
    return result.request;
}

static void check_transaction_fields(
    const ti_c54x_bus_word16_transaction *actual,
    const ti_c54x_bus_word16_transaction *expected)
{
    TI_C54X_TEST_CHECK(actual->state == expected->state);
    TI_C54X_TEST_CHECK(actual->transaction_id == expected->transaction_id);
    TI_C54X_TEST_CHECK(actual->request.space == expected->request.space);
    TI_C54X_TEST_CHECK(actual->request.access == expected->request.access);
    TI_C54X_TEST_CHECK(actual->request.address_word ==
                       expected->request.address_word);
    TI_C54X_TEST_CHECK(actual->request.write_value_word ==
                       expected->request.write_value_word);
    TI_C54X_TEST_CHECK(actual->staged_read_value_word ==
                       expected->staged_read_value_word);
    TI_C54X_TEST_CHECK(actual->staged_bus_status == expected->staged_bus_status);
}

static void check_zero_issue_result(
    ti_c54x_bus_word16_transaction_issue_result result,
    ti_c54x_bus_word16_transaction_status status)
{
    TI_C54X_TEST_CHECK(result.status == status);
    TI_C54X_TEST_CHECK(result.transaction_id == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID);
    TI_C54X_TEST_CHECK(result.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID);
    TI_C54X_TEST_CHECK(result.request.address_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(result.request.write_value_word == UINT16_C(0));
}

static void check_zero_completion_result(
    ti_c54x_bus_word16_transaction_completion_result result,
    ti_c54x_bus_word16_transaction_status status)
{
    TI_C54X_TEST_CHECK(result.status == status);
    TI_C54X_TEST_CHECK(result.read_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
}

static void test_prepare_begin_and_repeated_wait(void)
{
    ti_c54x_bus_word16_transaction transaction = {0};
    const ti_c54x_bus_word16_request request = make_request(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    const uint64_t id = UINT64_C(1);
    ti_c54x_bus_word16_request mutable_request = request;
    ti_c54x_bus_word16_transaction_issue_result issue;
    ti_c54x_bus_word16_transaction_observation observation = {0};
    ti_c54x_bus_word16_transaction_completion_result completion;
    ti_c54x_bus_word16_transaction before;
    ti_c54x_bus_word16_transaction terminal_before;

    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &mutable_request, id) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    mutable_request.address_word = UINT32_C(0x4321);
    mutable_request.write_value_word = UINT16_C(0x1111);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY);
    issue = ti_c54x_bus_word16_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.status ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(issue.transaction_id == id);
    TI_C54X_TEST_CHECK(issue.request.space == request.space);
    TI_C54X_TEST_CHECK(issue.request.access == request.access);
    TI_C54X_TEST_CHECK(issue.request.address_word == request.address_word);
    TI_C54X_TEST_CHECK(issue.request.write_value_word ==
                       request.write_value_word);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT);
    before = transaction;
    issue = ti_c54x_bus_word16_transaction_begin(&transaction);
    check_zero_issue_result(
        issue, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &before);

    observation.transaction_id = id;
    observation.kind = TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_WAIT;
    observation.bus_status = TI_C54X_BUS_OK;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT);
    before = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT);
    check_transaction_fields(&transaction, &before);

    before = transaction;
    completion = ti_c54x_bus_word16_transaction_complete(&transaction, id);
    check_zero_completion_result(
        completion, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT);
    check_transaction_fields(&transaction, &before);
    completion = ti_c54x_bus_word16_transaction_complete(&transaction, 0);
    check_zero_completion_result(
        completion, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_transaction_fields(&transaction, &before);
    completion = ti_c54x_bus_word16_transaction_complete(&transaction,
                                                           id + UINT64_C(99));
    check_zero_completion_result(
        completion, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION);
    check_transaction_fields(&transaction, &before);

    {
        const uint16_t read_values[] = {UINT16_C(0), UINT16_C(0x1234),
                                         UINT16_C(0xffff)};
        size_t read_index;

        for (read_index = 0U;
             read_index < sizeof(read_values) / sizeof(read_values[0]);
             ++read_index) {
            observation.kind =
                TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_READY;
            observation.read_value_word = read_values[read_index];
            TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                                   &transaction, &observation) ==
                               TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
            TI_C54X_TEST_CHECK(transaction.state ==
                               TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETION_READY);
            completion = ti_c54x_bus_word16_transaction_complete(
                &transaction, observation.transaction_id);
            TI_C54X_TEST_CHECK(completion.status ==
                               TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
            TI_C54X_TEST_CHECK_U64(completion.read_value_word,
                                   read_values[read_index]);
            TI_C54X_TEST_CHECK(completion.bus_status == TI_C54X_BUS_OK);
            TI_C54X_TEST_CHECK(transaction.state ==
                               TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE);
            terminal_before = transaction;
            completion = ti_c54x_bus_word16_transaction_complete(
                &transaction, observation.transaction_id);
            check_zero_completion_result(
                completion,
                TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
            check_transaction_fields(&transaction, &terminal_before);
            TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                                   &transaction, &observation) ==
                               TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
            check_transaction_fields(&transaction, &terminal_before);

            if (read_index + 1U < sizeof(read_values) / sizeof(read_values[0])) {
                TI_C54X_TEST_CHECK(
                    ti_c54x_bus_word16_transaction_reset(&transaction) ==
                    TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
                TI_C54X_TEST_CHECK(
                    ti_c54x_bus_word16_transaction_prepare(
                        &transaction, &request, id + (uint64_t)read_index +
                        UINT64_C(1)) ==
                    TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
                issue = ti_c54x_bus_word16_transaction_begin(&transaction);
                TI_C54X_TEST_CHECK(issue.status ==
                                   TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
                observation.transaction_id = id + (uint64_t)read_index +
                                             UINT64_C(1);
            }
        }
    }

    {
        ti_c54x_bus_word16_transaction reused = {0};
        ti_c54x_bus_word16_transaction_observation old_observation = {
            UINT64_C(1),
            TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_WAIT,
            UINT16_C(0),
            TI_C54X_BUS_OK};
        ti_c54x_bus_word16_transaction_observation new_observation = {
            UINT64_C(2),
            TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_READY,
            UINT16_C(0),
            TI_C54X_BUS_OK};
        const ti_c54x_bus_word16_request loop_request = make_request(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM,
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);

        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                               &reused, &loop_request, UINT64_C(1)) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(
            ti_c54x_bus_word16_transaction_begin(&reused).status ==
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                               &reused, &new_observation) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION);
        new_observation.transaction_id = UINT64_C(1);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                               &reused, &new_observation) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(
            ti_c54x_bus_word16_transaction_complete(&reused, UINT64_C(1))
                .status == TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(&reused) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                               &reused, &loop_request, UINT64_C(2)) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(
            ti_c54x_bus_word16_transaction_begin(&reused).status ==
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                               &reused, &old_observation) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION);
    }
}

static void test_all_spaces_accesses_and_write_payload_canonicalization(void)
{
    const ti_c54x_bus_word16_request_space spaces[] = {
        TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM,
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        TI_C54X_BUS_WORD16_REQUEST_SPACE_IO};
    const ti_c54x_bus_word16_request_access accesses[] = {
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE};
    size_t space_index;
    size_t access_index;
    uint64_t id = UINT64_C(10);

    for (space_index = 0U; space_index < sizeof(spaces) / sizeof(spaces[0]);
         ++space_index) {
        for (access_index = 0U;
             access_index < sizeof(accesses) / sizeof(accesses[0]);
             ++access_index) {
            ti_c54x_bus_word16_transaction transaction = {0};
            const ti_c54x_bus_word16_request request =
                make_request(spaces[space_index], accesses[access_index]);
            ti_c54x_bus_word16_transaction_observation observation = {
                id,
                TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_READY,
                UINT16_C(0),
                TI_C54X_BUS_OK};
            ti_c54x_bus_word16_transaction_completion_result result;

            TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                                   &transaction, &request, id) ==
                               TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
            TI_C54X_TEST_CHECK(
                ti_c54x_bus_word16_transaction_begin(&transaction).status ==
                TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
            if (accesses[access_index] ==
                TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
                observation.read_value_word = UINT16_C(0xffff);
            }
            TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                                   &transaction, &observation) ==
                               TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
            result = ti_c54x_bus_word16_transaction_complete(&transaction, id);
            TI_C54X_TEST_CHECK(result.status ==
                               TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
            if (accesses[access_index] ==
                TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
                TI_C54X_TEST_CHECK(result.read_value_word == UINT16_MAX);
            } else {
                TI_C54X_TEST_CHECK(result.read_value_word == UINT16_C(0));
            }
            ++id;
        }
    }
}

static void test_write_ready_and_fault_paths(void)
{
    const ti_c54x_bus_word16_request write_request = make_request(
        TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    ti_c54x_bus_word16_transaction transaction = {0};
    ti_c54x_bus_word16_transaction_observation observation = {
        UINT64_C(50),
        TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_READY,
        UINT16_C(1),
        TI_C54X_BUS_OK};
    ti_c54x_bus_word16_transaction before;
    ti_c54x_bus_word16_transaction_completion_result completion;

    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &write_request,
                           observation.transaction_id) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_begin(&transaction).status ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    before = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_transaction_fields(&transaction, &before);

    observation.read_value_word = UINT16_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    before = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(&transaction) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &before);
    completion = ti_c54x_bus_word16_transaction_complete(
        &transaction, observation.transaction_id);
    TI_C54X_TEST_CHECK(completion.status ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(completion.read_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(completion.bus_status == TI_C54X_BUS_OK);

    transaction = (ti_c54x_bus_word16_transaction){0};
    observation.kind = TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_FAULT;
    observation.bus_status = TI_C54X_BUS_NO_CALLBACK;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &write_request,
                           observation.transaction_id) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_begin(&transaction).status ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT);
    completion = ti_c54x_bus_word16_transaction_complete(
        &transaction, observation.transaction_id);
    TI_C54X_TEST_CHECK(completion.status ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(completion.read_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(completion.bus_status == TI_C54X_BUS_NO_CALLBACK);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(&transaction) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
}

static void test_faults_and_reset_reuse_reject_old_observation(void)
{
    const ti_c54x_bus_status faults[] = {TI_C54X_BUS_UNMAPPED,
                                         TI_C54X_BUS_FAULT,
                                         TI_C54X_BUS_NO_CALLBACK};
    size_t index;

    for (index = 0U; index < sizeof(faults) / sizeof(faults[0]); ++index) {
        ti_c54x_bus_word16_transaction transaction = {0};
        const ti_c54x_bus_word16_request request = make_request(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM,
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
        ti_c54x_bus_word16_transaction_observation observation = {
            UINT64_C(100) + index,
            TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_FAULT,
            UINT16_C(0),
            faults[index]};
        ti_c54x_bus_word16_transaction_completion_result result;
        ti_c54x_bus_word16_transaction before_terminal;

        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                               &transaction, &request, observation.transaction_id) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(
            ti_c54x_bus_word16_transaction_begin(&transaction).status ==
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                               &transaction, &observation) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT);
        result = ti_c54x_bus_word16_transaction_complete(
            &transaction, observation.transaction_id);
        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT);
        TI_C54X_TEST_CHECK(result.bus_status == faults[index]);
        TI_C54X_TEST_CHECK(result.read_value_word == UINT16_C(0));
        before_terminal = transaction;
        result = ti_c54x_bus_word16_transaction_complete(
            &transaction, observation.transaction_id);
        check_zero_completion_result(
            result, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
        check_transaction_fields(&transaction, &before_terminal);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                               &transaction, &observation) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
        check_transaction_fields(&transaction, &before_terminal);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(&transaction) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(transaction.state ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE);
    }

    {
        ti_c54x_bus_word16_transaction transaction = {0};
        const ti_c54x_bus_word16_request request = make_request(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA,
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
        ti_c54x_bus_word16_transaction_observation old_observation = {
            UINT64_C(200),
            TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_WAIT,
            UINT16_C(0),
            TI_C54X_BUS_OK};
        ti_c54x_bus_word16_transaction_observation ready_observation = {
            UINT64_C(201),
            TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_READY,
            UINT16_C(0xbeef),
            TI_C54X_BUS_OK};

        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                               &transaction, &request, old_observation.transaction_id) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(&transaction) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
        TI_C54X_TEST_CHECK(
            ti_c54x_bus_word16_transaction_begin(&transaction).status ==
            TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                               &transaction, &old_observation) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(&transaction) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                               &transaction, &ready_observation) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION);
    }
}

int main(void)
{
    test_prepare_begin_and_repeated_wait();
    test_all_spaces_accesses_and_write_payload_canonicalization();
    test_write_ready_and_fault_paths();
    test_faults_and_reset_reuse_reject_old_observation();
    return 0;
}
