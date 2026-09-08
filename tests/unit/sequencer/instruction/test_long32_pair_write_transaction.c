/*
 * Module contract
 *
 * Owns: executable LONG32 pair WRITE lifecycle contracts for the private
 * sequencer reducer. It proves copied MSW/LSW values, exact issue order,
 * WAIT/FAULT/cancel behavior, deferred completion, and explicit per-word
 * endpoint visibility without claiming physical rollback.
 * Does not own: endpoint arbitration, CPU state, opcode decode, address
 * planning, timing, or external-master exclusion.
 */

#include "sequencer/instruction/long32_pair_transaction.h"
#include "address/data_operand.h"
#include "../../../support/test_harness.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct write_visibility_model {
    unsigned issued;
    uint16_t issued_words[2];
    uint64_t issued_ids[2];
    unsigned writes;
    uint16_t words[2];
} write_visibility_model;

static ti_c54x_long32_pair_request
write_request(void)
{
    ti_c54x_long32_pair_request request = {0};

    request.operand = ti_c54x_data_operand_long32(UINT16_C(0x0501));
    request.access = TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE;
    request.first_value_msw = UINT16_C(0x1234);
    request.second_value_lsw = UINT16_C(0x5678);
    request.first_transaction_id = UINT64_C(8001);
    request.second_transaction_id = UINT64_C(8002);
    return request;
}

static ti_c54x_bus_word16_observation
ready(uint64_t transaction_id)
{
    return (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .read_value_word = UINT16_C(0),
        .bus_status = TI_C54X_BUS_OK};
}

static ti_c54x_bus_word16_observation
wait_observation(uint64_t transaction_id)
{
    return (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT,
        .read_value_word = UINT16_C(0),
        .bus_status = TI_C54X_BUS_OK};
}

static ti_c54x_bus_word16_observation
fault(uint64_t transaction_id)
{
    return (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT,
        .read_value_word = UINT16_C(0),
        .bus_status = TI_C54X_BUS_FAULT};
}

static ti_c54x_long32_pair_transaction
prepared_transaction(void)
{
    ti_c54x_long32_pair_transaction transaction = {0};
    const ti_c54x_long32_pair_request request = write_request();

    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_prepare(
                           &transaction, &request) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    return transaction;
}

static void
model_issue(const ti_c54x_long32_pair_issue_result *issue,
            write_visibility_model *model)
{
    TI_C54X_TEST_CHECK(issue->request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(model->issued < 2U);
    model->issued_words[model->issued] = issue->request.write_value_word;
    model->issued_ids[model->issued] = issue->transaction_id;
    ++model->issued;
}

static void
model_ready(write_visibility_model *model)
{
    TI_C54X_TEST_CHECK(model->writes < model->issued);
    TI_C54X_TEST_CHECK(model->writes < 2U);
    model->words[model->writes] = model->issued_words[model->writes];
    ++model->writes;
}

static void
test_exact_values_order_and_final_completion(void)
{
    ti_c54x_long32_pair_transaction transaction = prepared_transaction();
    ti_c54x_long32_pair_issue_result issue;
    ti_c54x_long32_pair_completion_result completion;
    write_visibility_model model = {0};
    const ti_c54x_bus_word16_observation first_ready = ready(UINT64_C(8001));
    const ti_c54x_bus_word16_observation second_ready = ready(UINT64_C(8002));

    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.status == TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(8001));
    TI_C54X_TEST_CHECK(issue.request.address_word == UINT32_C(0x0501));
    TI_C54X_TEST_CHECK(issue.request.write_value_word == UINT16_C(0x1234));
    model_issue(&issue, &model);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &first_ready) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    model_ready(&model);
    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.status == TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(8002));
    TI_C54X_TEST_CHECK(issue.request.address_word == UINT32_C(0x0500));
    TI_C54X_TEST_CHECK(issue.request.write_value_word == UINT16_C(0x5678));
    model_issue(&issue, &model);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &second_ready) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    model_ready(&model);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_LONG32_PAIR_STATE_COMPLETION_READY);
    completion = ti_c54x_long32_pair_transaction_complete(&transaction);
    TI_C54X_TEST_CHECK(completion.status == TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(completion.first_msw == UINT16_C(0));
    TI_C54X_TEST_CHECK(completion.second_lsw == UINT16_C(0));
    TI_C54X_TEST_CHECK(model.writes == 2U);
    TI_C54X_TEST_CHECK(model.words[0] == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(model.words[1] == UINT16_C(0x5678));
}

static void
test_wait_and_cancel_preserve_active_word(void)
{
    ti_c54x_long32_pair_transaction transaction = prepared_transaction();
    ti_c54x_bus_word16_transaction child_snapshot;
    ti_c54x_long32_pair_transaction snapshot;
    ti_c54x_long32_pair_issue_result issue;
    write_visibility_model model = {0};
    const ti_c54x_bus_word16_observation first_wait =
        wait_observation(UINT64_C(8001));

    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    model_issue(&issue, &model);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &first_wait) ==
                       TI_C54X_LONG32_PAIR_STATUS_WAIT);
    child_snapshot = transaction.word16_transaction;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &first_wait) ==
                       TI_C54X_LONG32_PAIR_STATUS_WAIT);
    TI_C54X_TEST_CHECK(memcmp(&transaction.word16_transaction,
                              &child_snapshot,
                              sizeof(child_snapshot)) == 0);
    snapshot = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_cancel(
                           &transaction, UINT64_C(8000)) ==
                       TI_C54X_LONG32_PAIR_STATUS_STALE_TRANSACTION);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_cancel(
                           &transaction, UINT64_C(8001)) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.state == TI_C54X_LONG32_PAIR_STATE_IDLE);
    TI_C54X_TEST_CHECK(model.writes == 0U);
}

static void
test_fault_does_not_rollback_prior_word(void)
{
    ti_c54x_long32_pair_transaction transaction = prepared_transaction();
    ti_c54x_long32_pair_issue_result issue;
    ti_c54x_long32_pair_completion_result completion;
    write_visibility_model model = {0};
    const ti_c54x_bus_word16_observation first_ready = ready(UINT64_C(8001));
    const ti_c54x_bus_word16_observation first_fault = fault(UINT64_C(8001));
    const ti_c54x_bus_word16_observation second_fault = fault(UINT64_C(8002));

    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    model_issue(&issue, &model);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &first_ready) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    model_ready(&model);
    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    model_issue(&issue, &model);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &second_fault) ==
                       TI_C54X_LONG32_PAIR_STATUS_FAULT);
    completion = ti_c54x_long32_pair_transaction_complete(&transaction);
    TI_C54X_TEST_CHECK(completion.status == TI_C54X_LONG32_PAIR_STATUS_FAULT);
    TI_C54X_TEST_CHECK(completion.first_msw == UINT16_C(0));
    TI_C54X_TEST_CHECK(completion.second_lsw == UINT16_C(0));
    TI_C54X_TEST_CHECK(model.writes == 1U);
    TI_C54X_TEST_CHECK(model.words[0] == UINT16_C(0x1234));

    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_reset(&transaction) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    transaction = prepared_transaction();
    model = (write_visibility_model){0};
    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    model_issue(&issue, &model);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &first_fault) ==
                       TI_C54X_LONG32_PAIR_STATUS_FAULT);
    TI_C54X_TEST_CHECK(model.writes == 0U);
    TI_C54X_TEST_CHECK(transaction.state == TI_C54X_LONG32_PAIR_STATE_FAULT);
}

static void
test_reset_only_terminal_and_cancel_second(void)
{
    ti_c54x_long32_pair_transaction transaction = prepared_transaction();
    ti_c54x_long32_pair_transaction snapshot;
    ti_c54x_long32_pair_issue_result issue;
    write_visibility_model model = {0};
    const ti_c54x_bus_word16_observation first_ready = ready(UINT64_C(8001));

    snapshot = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_reset(&transaction) ==
                       TI_C54X_LONG32_PAIR_STATUS_INVALID_STATE);
    TI_C54X_TEST_CHECK(memcmp(&transaction, &snapshot, sizeof(transaction)) ==
                       0);
    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    model_issue(&issue, &model);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_poll(
                           &transaction, &first_ready) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    model_ready(&model);
    issue = ti_c54x_long32_pair_transaction_begin(&transaction);
    TI_C54X_TEST_CHECK(issue.transaction_id == UINT64_C(8002));
    model_issue(&issue, &model);
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_cancel(
                           &transaction, UINT64_C(8002)) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.state == TI_C54X_LONG32_PAIR_STATE_IDLE);
    TI_C54X_TEST_CHECK(model.writes == 1U);
    TI_C54X_TEST_CHECK(model.words[0] == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(ti_c54x_long32_pair_transaction_reset(&transaction) ==
                       TI_C54X_LONG32_PAIR_STATUS_OK);
}

int
main(void)
{
    test_exact_values_order_and_final_completion();
    test_wait_and_cancel_preserve_active_word();
    test_fault_does_not_rollback_prior_word();
    test_reset_only_terminal_and_cancel_second();
    puts("long32_pair_write_transaction: ok");
    return 0;
}
