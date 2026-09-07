/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proof for cancellation in the
 * private value-owned WORD16 transaction reducer.
 * Does not own: endpoint callbacks, physical visibility, adapter policy,
 * CPU reset/deinitialization, address effects, devices, timing, diagnostics,
 * or test discovery outside this owner.
 */

#include "../../../../src/bus/request/word16_request.h"
#include "../../../../src/bus/transaction/word16_transaction.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

static ti_c54x_bus_word16_request valid_request(void)
{
    const ti_c54x_bus_word16_request_result result =
        ti_c54x_bus_word16_request_write(
            TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, UINT32_C(0x1234),
            UINT16_C(0xabcd));

    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_REQUEST_STATUS_OK);
    return result.request;
}

static ti_c54x_bus_word16_transaction active_transaction(uint64_t id)
{
    ti_c54x_bus_word16_transaction transaction = {0};
    const ti_c54x_bus_word16_request request = valid_request();

    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &request, id) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_begin(&transaction).status ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    return transaction;
}

static void check_same_transaction(
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
    TI_C54X_TEST_CHECK(actual->staged_bus_status ==
                       expected->staged_bus_status);
}

static void test_matching_active_id_cancels_and_clears(void)
{
    ti_c54x_bus_word16_transaction transaction = active_transaction(21);

    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_cancel(
                           &transaction, UINT64_C(21)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE);
    TI_C54X_TEST_CHECK(transaction.transaction_id == UINT64_C(0));
    TI_C54X_TEST_CHECK(transaction.request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_INVALID);
    TI_C54X_TEST_CHECK(transaction.request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_INVALID);
    TI_C54X_TEST_CHECK(transaction.request.address_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(transaction.request.write_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(transaction.staged_read_value_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(transaction.staged_bus_status == TI_C54X_BUS_OK);
}

static void test_stale_and_zero_ids_preserve_active_state(void)
{
    ti_c54x_bus_word16_transaction transaction = active_transaction(22);
    const ti_c54x_bus_word16_transaction before = transaction;

    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_cancel(
                           &transaction, UINT64_C(0)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
    check_same_transaction(&transaction, &before);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_cancel(
                           &transaction, UINT64_C(23)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_STALE_TRANSACTION);
    check_same_transaction(&transaction, &before);
}

static void test_idle_and_terminal_states_reject_cancel_without_mutation(void)
{
    ti_c54x_bus_word16_transaction idle = {0};
    ti_c54x_bus_word16_transaction complete = active_transaction(24);
    ti_c54x_bus_word16_transaction fault = active_transaction(25);
    const ti_c54x_bus_word16_transaction idle_before = idle;
    ti_c54x_bus_word16_transaction complete_before;
    ti_c54x_bus_word16_transaction fault_before;
    ti_c54x_bus_word16_observation observation = {
        UINT64_C(25), TI_C54X_BUS_WORD16_OBSERVATION_FAULT, UINT16_C(0),
        TI_C54X_BUS_FAULT};

    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_cancel(
                           &idle, UINT64_C(1)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_same_transaction(&idle, &idle_before);

    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(
                           &complete, &(ti_c54x_bus_word16_observation){
                               UINT64_C(24),
                               TI_C54X_BUS_WORD16_OBSERVATION_READY,
                               UINT16_C(0), TI_C54X_BUS_OK}) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_complete(&complete, UINT64_C(24)).status ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    complete_before = complete;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_cancel(
                           &complete, UINT64_C(24)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_same_transaction(&complete, &complete_before);

    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(&fault,
                                                            &observation) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT);
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_transaction_complete(&fault, UINT64_C(25)).status ==
        TI_C54X_BUS_WORD16_TRANSACTION_STATUS_FAULT);
    fault_before = fault;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_cancel(
                           &fault, UINT64_C(25)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_STATE);
    check_same_transaction(&fault, &fault_before);
}

static void test_null_transaction_is_deterministic(void)
{
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_cancel(
                           NULL, UINT64_C(1)) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_INVALID_ARGUMENT);
}

int main(void)
{
    test_matching_active_id_cancels_and_clears();
    test_stale_and_zero_ids_preserve_active_state();
    test_idle_and_terminal_states_reject_cancel_without_mutation();
    test_null_transaction_is_deterministic();
    return 0;
}
