/*
 * Module contract
 *
 * Owns: adversarial PROJECT_CONTRACT_ONLY validation proof for the private
 * WORD16 transaction reducer and its split validator.
 * Does not own: external bus issue counts, memory visibility, arbitration,
 * device timing, public ABI, or test discovery.
 */

#include "../../../../src/bus/request/word16_request.h"
#include "../../../../src/bus/transaction/word16_transaction.h"
#include "../../../../src/bus/transaction/word16_transaction_validate.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static ti_c54x_bus_word16_request valid_request(void)
{
    return ti_c54x_bus_word16_request_read(
               TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, UINT32_C(0x20))
        .request;
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

static void check_observation_fields(
    const ti_c54x_bus_word16_transaction_observation *actual,
    const ti_c54x_bus_word16_transaction_observation *expected)
{
    TI_C54X_TEST_CHECK(actual->transaction_id == expected->transaction_id);
    TI_C54X_TEST_CHECK(actual->kind == expected->kind);
    TI_C54X_TEST_CHECK(actual->read_value_word == expected->read_value_word);
    TI_C54X_TEST_CHECK(actual->bus_status == expected->bus_status);
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

static void test_validator_rejects_forged_shapes(void)
{
    ti_c54x_bus_word16_transaction_observation observation = {0};
    ti_c54x_bus_word16_request request = valid_request();
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_state_is_valid(
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE));
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_state_is_valid(
            TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT));
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_transaction_state_is_valid(
        (ti_c54x_bus_word16_transaction_state)UINT8_C(0x7f)));
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_request_is_valid(&request));
    request.write_value_word = UINT16_C(1);
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_transaction_request_is_valid(
        &request));
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_transaction_request_is_valid(NULL));
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_observation_is_valid(&observation) ==
        false);
    observation.kind =
        (ti_c54x_bus_word16_transaction_observation_kind)UINT8_C(0x7f);
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_transaction_observation_is_valid(
        &observation));
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_transaction_observation_is_valid(
        NULL));
}

static void test_observation_payload_validation_and_precedence(void)
{
    ti_c54x_bus_word16_transaction transaction = {0};
    const ti_c54x_bus_word16_request request = valid_request();
    const ti_c54x_bus_word16_transaction_observation baseline = {
        UINT64_C(9),
        TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_WAIT,
        UINT16_C(0),
        TI_C54X_BUS_OK};
    ti_c54x_bus_word16_transaction_observation observation = baseline;
    ti_c54x_bus_word16_transaction before;
    ti_c54x_bus_word16_transaction_observation borrowed_before;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &request, UINT64_C(9)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_begin(&transaction).status ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    before = transaction;

    borrowed_before = baseline;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT);
    check_transaction_fields(&transaction, &before);
    check_observation_fields(&observation, &borrowed_before);

    observation.transaction_id = UINT64_C(0);
    borrowed_before = observation;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_transaction_fields(&transaction, &before);
    check_observation_fields(&observation, &borrowed_before);
    observation = baseline;
    observation.bus_status = TI_C54X_BUS_FAULT;
    borrowed_before = observation;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_transaction_fields(&transaction, &before);
    check_observation_fields(&observation, &borrowed_before);
    observation = baseline;
    observation.kind = TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_FAULT;
    observation.bus_status = TI_C54X_BUS_OK;
    borrowed_before = observation;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_transaction_fields(&transaction, &before);
    check_observation_fields(&observation, &borrowed_before);
    observation = baseline;
    observation.kind = TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_READY;
    observation.read_value_word = UINT16_C(1);
    observation.bus_status = TI_C54X_BUS_OK;
    borrowed_before = observation;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    check_observation_fields(&observation, &borrowed_before);

    transaction = (ti_c54x_bus_word16_transaction){0};
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &request, UINT64_C(9)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_begin(&transaction).status ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    observation = baseline;
    observation.kind = TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_INVALID;
    borrowed_before = observation;
    before = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_transaction_fields(&transaction, &before);
    check_observation_fields(&observation, &borrowed_before);
    observation.kind =
        (ti_c54x_bus_word16_transaction_observation_kind)UINT8_C(0x7f);
    borrowed_before = observation;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_transaction_fields(&transaction, &before);
    check_observation_fields(&observation, &borrowed_before);

    {
        const ti_c54x_bus_status forged_statuses[] = {
            TI_C54X_BUS_UNMAPPED, TI_C54X_BUS_FAULT,
            TI_C54X_BUS_NO_CALLBACK};
        size_t index;

        for (index = 0U;
             index < sizeof(forged_statuses) / sizeof(forged_statuses[0]);
             ++index) {
            observation = baseline;
            observation.bus_status = forged_statuses[index];
            borrowed_before = observation;
            TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                                   &transaction, &observation) ==
                               TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
            check_transaction_fields(&transaction, &before);
            check_observation_fields(&observation, &borrowed_before);
        }

        observation = baseline;
        observation.bus_status = (ti_c54x_bus_status)UINT8_C(0x7f);
        borrowed_before = observation;
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                               &transaction, &observation) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
        check_transaction_fields(&transaction, &before);
        check_observation_fields(&observation, &borrowed_before);
    }

    {
        const ti_c54x_bus_word16_transaction_observation invalid_payloads[] = {
            {UINT64_C(9), TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_WAIT,
             UINT16_C(1), TI_C54X_BUS_OK},
            {UINT64_C(9), TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_READY,
             UINT16_C(0), TI_C54X_BUS_FAULT},
            {UINT64_C(9), TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_FAULT,
             UINT16_C(1), TI_C54X_BUS_FAULT},
            {UINT64_C(9), TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_FAULT,
             UINT16_C(0), (ti_c54x_bus_status)UINT8_C(0x7f)}};
        size_t index;

        for (index = 0U;
             index < sizeof(invalid_payloads) / sizeof(invalid_payloads[0]);
             ++index) {
            observation = invalid_payloads[index];
            borrowed_before = observation;
            TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                                   &transaction, &observation) ==
                               TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
            check_transaction_fields(&transaction, &before);
            check_observation_fields(&observation, &borrowed_before);
        }
    }

    observation = baseline;
    observation.transaction_id = UINT64_C(8);
    observation.kind =
        (ti_c54x_bus_word16_transaction_observation_kind)UINT8_C(0x7f);
    observation.bus_status = (ti_c54x_bus_status)UINT8_C(0x7f);
    borrowed_before = observation;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION);
    check_transaction_fields(&transaction, &before);
    check_observation_fields(&observation, &borrowed_before);

    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           NULL, NULL) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, NULL) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_transaction_fields(&transaction, &before);
}

static void test_state_shape_validation_is_transactional(void)
{
    const ti_c54x_bus_word16_request request = valid_request();
    ti_c54x_bus_word16_transaction transaction = {0};
    ti_c54x_bus_word16_transaction expected;
    ti_c54x_bus_word16_transaction_issue_result issue;
    ti_c54x_bus_word16_transaction_completion_result completion;
    ti_c54x_bus_word16_transaction_observation ready = {
        UINT64_C(7),
        TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_READY,
        UINT16_C(0x1234),
        TI_C54X_BUS_OK};
    ti_c54x_bus_word16_transaction_observation fault = {
        UINT64_C(7),
        TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_FAULT,
        UINT16_C(0),
        TI_C54X_BUS_FAULT};

    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_transaction_is_canonical(NULL));
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_is_canonical(&transaction));
    transaction.transaction_id = UINT64_C(7);
    expected = transaction;
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_transaction_is_canonical(
        &transaction));
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &request, UINT64_C(8)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &expected);
    issue = ti_c54x_bus_word16_transaction_begin(&transaction);
    check_zero_issue_result(
        issue, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &expected);

    transaction = (ti_c54x_bus_word16_transaction){0};
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &request, UINT64_C(7)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    transaction.staged_read_value_word = UINT16_C(1);
    expected = transaction;
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_transaction_is_canonical(
        &transaction));
    issue = ti_c54x_bus_word16_transaction_begin(&transaction);
    check_zero_issue_result(
        issue, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &expected);

    transaction = (ti_c54x_bus_word16_transaction){0};
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &request, UINT64_C(7)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_begin(&transaction).status ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    transaction.request.write_value_word = UINT16_C(1);
    expected = transaction;
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_transaction_is_canonical(
        &transaction));
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &ready) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &expected);

    transaction.request = request;
    transaction.staged_bus_status = TI_C54X_BUS_FAULT;
    expected = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &ready) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &expected);

    transaction.staged_bus_status = TI_C54X_BUS_OK;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &ready) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    transaction.staged_bus_status = TI_C54X_BUS_FAULT;
    expected = transaction;
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_transaction_is_canonical(
        &transaction));
    completion = ti_c54x_bus_word16_transaction_complete(
        &transaction, UINT64_C(7));
    check_zero_completion_result(
        completion, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &expected);

    transaction.staged_bus_status = TI_C54X_BUS_OK;
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_complete(&transaction, UINT64_C(7))
            .status == TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    transaction.staged_bus_status = TI_C54X_BUS_FAULT;
    expected = transaction;
    TI_C54X_TEST_CHECK(!ti_c54x_bus_word16_transaction_is_canonical(
        &transaction));
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(&transaction) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &expected);
    transaction = (ti_c54x_bus_word16_transaction){0};
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(&transaction) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_prepare(&transaction, &request,
                                               UINT64_C(7)) ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_begin(&transaction).status ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &fault) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT);
    completion = ti_c54x_bus_word16_transaction_complete(
        &transaction, UINT64_C(7));
    TI_C54X_TEST_CHECK(completion.status ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(completion.bus_status == TI_C54X_BUS_FAULT);
    transaction.staged_bus_status = TI_C54X_BUS_OK;
    expected = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(&transaction) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &expected);
}

static void test_nulls_states_ids_and_failed_outputs(void)
{
    ti_c54x_bus_word16_transaction transaction = {0};
    const ti_c54x_bus_word16_request request = valid_request();
    ti_c54x_bus_word16_request invalid_request = request;
    ti_c54x_bus_word16_transaction before;
    ti_c54x_bus_word16_transaction_observation observation = {0};
    ti_c54x_bus_word16_transaction_issue_result issue;
    ti_c54x_bus_word16_transaction_completion_result completion;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           NULL, &request, UINT64_C(1)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, NULL, UINT64_C(1)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &request, UINT64_C(0)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    invalid_request.write_value_word = UINT16_C(1);
    before = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &invalid_request, UINT64_C(1)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_transaction_fields(&transaction, &before);
    issue = ti_c54x_bus_word16_transaction_begin(NULL);
    check_zero_issue_result(
        issue, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    completion = ti_c54x_bus_word16_transaction_complete(NULL, UINT64_C(1));
    check_zero_completion_result(
        completion, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(NULL) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);

    transaction.state =
        (ti_c54x_bus_word16_transaction_state)UINT8_C(0x7f);
    before = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, NULL) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_transaction_fields(&transaction, &before);
    issue = ti_c54x_bus_word16_transaction_begin(&transaction);
    check_zero_issue_result(
        issue, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    completion = ti_c54x_bus_word16_transaction_complete(&transaction, 0);
    check_zero_completion_result(
        completion, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    observation.transaction_id = UINT64_C(0);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &transaction, &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(&transaction) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &before);
}

static void test_completion_precedence_and_nonmutation(void)
{
    const ti_c54x_bus_word16_request request = valid_request();
    ti_c54x_bus_word16_transaction transaction = {0};
    ti_c54x_bus_word16_transaction expected;
    ti_c54x_bus_word16_transaction_completion_result completion;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &request, UINT64_C(31)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_begin(&transaction).status ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    expected = transaction;
    completion = ti_c54x_bus_word16_transaction_complete(&transaction,
                                                           UINT64_C(31));
    check_zero_completion_result(
        completion, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_WAIT);
    check_transaction_fields(&transaction, &expected);
    completion = ti_c54x_bus_word16_transaction_complete(&transaction, 0);
    check_zero_completion_result(
        completion, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_transaction_fields(&transaction, &expected);
    completion = ti_c54x_bus_word16_transaction_complete(&transaction,
                                                           UINT64_C(32));
    check_zero_completion_result(
        completion, TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION);
    check_transaction_fields(&transaction, &expected);

    transaction = (ti_c54x_bus_word16_transaction){0};
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &request, UINT64_C(31)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_begin(&transaction).status ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    {
        const ti_c54x_bus_word16_transaction_observation ready = {
            UINT64_C(31),
            TI_C54X_BUS_WORD16_TRANSACTION_OBSERVATION_READY,
            UINT16_C(0),
            TI_C54X_BUS_OK};
        TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                               &transaction, &ready) ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    }
    expected = transaction;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_reset(&transaction) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_transaction_fields(&transaction, &expected);
}

int main(void)
{
    test_validator_rejects_forged_shapes();
    test_observation_payload_validation_and_precedence();
    test_state_shape_validation_is_transactional();
    test_nulls_states_ids_and_failed_outputs();
    test_completion_precedence_and_nonmutation();
    return 0;
}
