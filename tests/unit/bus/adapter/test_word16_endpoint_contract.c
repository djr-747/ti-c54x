/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proof of the copied public DATA
 * WORD16 endpoint descriptor and its callback protocol.
 * Does not own: endpoint implementation, physical storage, byte order,
 * arbitration, device policy, CPU lifecycle, instruction sequencing,
 * diagnostics, timing, or test discovery outside this owner.
 *
 * The fixture deliberately models an external adapter.  It records every
 * callback argument and only publishes a write when it returns READY; this
 * keeps visibility and callback-count assertions independent of the core's
 * private reducer.
 */

#include "../../../../src/bus/adapter/word16_adapter.h"
#include "../../../../src/bus/request/word16_request.h"
#include "../../../../src/bus/transaction/word16_transaction.h"
#include "../../../support/test_harness.h"

#include <ti_c54x/bus_endpoint.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum endpoint_mode {
    ENDPOINT_READ_READY = 0,
    ENDPOINT_READ_WAIT_THEN_READY = 1,
    ENDPOINT_WRITE_READY = 2,
    ENDPOINT_WRITE_WAIT_THEN_READY = 3,
    ENDPOINT_FAULT = 4,
    ENDPOINT_INVALID_OBSERVATION = 5,
    ENDPOINT_STALE_OBSERVATION = 6
} endpoint_mode;

typedef struct endpoint_fixture {
    endpoint_mode mode;
    unsigned int callback_count;
    unsigned int begin_count;
    unsigned int resume_count;
    unsigned int cancel_count;
    unsigned int visible_write_count;
    unsigned int zeroed_observation_count;
    uint64_t ids[3];
    ti_c54x_bus_word16_request requests[3];
    uint16_t read_value_word;
    ti_c54x_bus_status fault_status;
    ti_c54x_bus_word16_cancel_result cancel_result;
} endpoint_fixture;

static ti_c54x_bus_word16_request make_request(
    ti_c54x_bus_word16_request_access access)
{
    const ti_c54x_bus_word16_request_result result =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
            ? ti_c54x_bus_word16_request_read(
                  TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, UINT32_C(0x2345))
            : ti_c54x_bus_word16_request_write(
                  TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, UINT32_C(0x2345),
                  UINT16_C(0xbeef));

    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_REQUEST_STATUS_OK);
    return result.request;
}

static void check_request_fields(
    const ti_c54x_bus_word16_request *actual,
    const ti_c54x_bus_word16_request *expected)
{
    TI_C54X_TEST_CHECK(actual->space == expected->space);
    TI_C54X_TEST_CHECK(actual->access == expected->access);
    TI_C54X_TEST_CHECK(actual->address_word == expected->address_word);
    TI_C54X_TEST_CHECK(actual->write_value_word == expected->write_value_word);
}

static void endpoint_advance(void *context,
                              ti_c54x_bus_word16_advance_phase phase,
                              uint64_t transaction_id,
                              const ti_c54x_bus_word16_request *request,
                              ti_c54x_bus_word16_observation *observation)
{
    endpoint_fixture *fixture = (endpoint_fixture *)context;
    const unsigned int index = fixture->callback_count;
    bool wait;

    TI_C54X_TEST_CHECK(index < sizeof(fixture->ids) / sizeof(fixture->ids[0]));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    if (observation->transaction_id == UINT64_C(0) &&
        observation->kind == TI_C54X_BUS_WORD16_OBSERVATION_INVALID &&
        observation->read_value_word == UINT16_C(0) &&
        observation->bus_status == TI_C54X_BUS_OK) {
        fixture->zeroed_observation_count += 1U;
    }
    fixture->ids[index] = transaction_id;
    fixture->requests[index] = *request;
    fixture->callback_count += 1U;
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        fixture->begin_count += 1U;
    } else if (phase == TI_C54X_BUS_WORD16_ADVANCE_RESUME) {
        fixture->resume_count += 1U;
    } else {
        TI_C54X_TEST_CHECK(false);
    }

    wait = fixture->mode == ENDPOINT_READ_WAIT_THEN_READY ||
           fixture->mode == ENDPOINT_WRITE_WAIT_THEN_READY;
    if (wait && fixture->callback_count < 3U) {
        observation->transaction_id = transaction_id;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        observation->read_value_word = UINT16_C(0);
        observation->bus_status = TI_C54X_BUS_OK;
        return;
    }
    if (fixture->mode == ENDPOINT_FAULT) {
        observation->transaction_id = transaction_id;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->read_value_word = UINT16_C(0);
        observation->bus_status = fixture->fault_status;
        return;
    }
    if (fixture->mode == ENDPOINT_INVALID_OBSERVATION) {
        observation->transaction_id = transaction_id;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_INVALID;
        observation->read_value_word = UINT16_C(0);
        observation->bus_status = TI_C54X_BUS_OK;
        return;
    }
    observation->transaction_id = fixture->mode == ENDPOINT_STALE_OBSERVATION
                                      ? transaction_id + UINT64_C(1)
                                      : transaction_id;
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
    observation->read_value_word = fixture->read_value_word;
    observation->bus_status = TI_C54X_BUS_OK;
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
        TI_C54X_TEST_CHECK(observation->read_value_word == UINT16_C(0));
        fixture->visible_write_count += 1U;
    }
}

static ti_c54x_bus_word16_cancel_result endpoint_cancel(void *context,
                                                        uint64_t transaction_id,
                                                        const ti_c54x_bus_word16_request *request)
{
    endpoint_fixture *fixture = (endpoint_fixture *)context;

    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    fixture->cancel_count += 1U;
    return fixture->cancel_result;
}

static ti_c54x_bus_word16_endpoint make_endpoint(endpoint_fixture *fixture)
{
    ti_c54x_bus_word16_endpoint endpoint = {0};

    endpoint.context = fixture;
    endpoint.advance = endpoint_advance;
    endpoint.cancel = endpoint_cancel;
    return endpoint;
}

static ti_c54x_bus_word16_transaction prepare_transaction(
    ti_c54x_bus_word16_request_access access,
    uint64_t transaction_id)
{
    ti_c54x_bus_word16_transaction transaction = {0};
    const ti_c54x_bus_word16_request request = make_request(access);

    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_prepare(
                           &transaction, &request, transaction_id) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    return transaction;
}

static void test_canonical_absent_and_partial_endpoints_are_rejected(void)
{
    endpoint_fixture fixture = {0};
    const ti_c54x_bus_word16_endpoint absent = {0};
    ti_c54x_bus_word16_endpoint partial = make_endpoint(&fixture);
    ti_c54x_bus_word16_endpoint partial_without_advance = make_endpoint(&fixture);
    ti_c54x_bus_word16_transaction transaction = prepare_transaction(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT64_C(1));
    ti_c54x_bus_word16_adapter_result result;

    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_adapter_validate_endpoint(NULL) ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_adapter_validate_endpoint(&absent) ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_NO_CALLBACK);
    partial.cancel = NULL;
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_adapter_validate_endpoint(&partial) ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ENDPOINT);
    result = ti_c54x_bus_word16_adapter_advance(
        &absent, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_NO_CALLBACK);
    TI_C54X_TEST_CHECK(fixture.callback_count == 0U);
    result = ti_c54x_bus_word16_adapter_advance(
        &partial, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ENDPOINT);
    TI_C54X_TEST_CHECK(fixture.callback_count == 0U);
    partial_without_advance.advance = NULL;
    TI_C54X_TEST_CHECK(
        ti_c54x_bus_word16_adapter_validate_endpoint(&partial_without_advance) ==
        TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ENDPOINT);
    result = ti_c54x_bus_word16_adapter_advance(
        &partial_without_advance, TI_C54X_BUS_WORD16_ADVANCE_BEGIN,
        &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ENDPOINT);
    TI_C54X_TEST_CHECK(fixture.callback_count == 0U);
}

static void test_immediate_read_ready_is_one_callback_and_one_completion(void)
{
    endpoint_fixture fixture = {
        .mode = ENDPOINT_READ_READY,
        .read_value_word = UINT16_C(0xa55a),
        .fault_status = TI_C54X_BUS_FAULT,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_bus_word16_transaction transaction = prepare_transaction(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT64_C(7));
    const ti_c54x_bus_word16_request expected = transaction.request;
    ti_c54x_bus_word16_adapter_result result;

    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.read_value_word == UINT16_C(0xa55a));
    TI_C54X_TEST_CHECK(fixture.callback_count == 1U);
    TI_C54X_TEST_CHECK(fixture.zeroed_observation_count == 1U);
    TI_C54X_TEST_CHECK(fixture.begin_count == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_count == 0U);
    TI_C54X_TEST_CHECK(fixture.ids[0] == UINT64_C(7));
    check_request_fields(&fixture.requests[0], &expected);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(fixture.callback_count == 1U);
}

static void test_wait_resume_reuses_id_and_request_without_reissue(void)
{
    endpoint_fixture fixture = {
        .mode = ENDPOINT_READ_WAIT_THEN_READY,
        .read_value_word = UINT16_C(0x1234),
        .fault_status = TI_C54X_BUS_FAULT,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_bus_word16_transaction transaction = prepare_transaction(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT64_C(8));
    const ti_c54x_bus_word16_request expected = transaction.request;
    ti_c54x_bus_word16_adapter_result result;

    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK);
    TI_C54X_TEST_CHECK(result.read_value_word == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(fixture.callback_count == 3U);
    TI_C54X_TEST_CHECK(fixture.zeroed_observation_count == 3U);
    TI_C54X_TEST_CHECK(fixture.begin_count == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_count == 2U);
    TI_C54X_TEST_CHECK(fixture.ids[0] == UINT64_C(8));
    TI_C54X_TEST_CHECK(fixture.ids[1] == UINT64_C(8));
    TI_C54X_TEST_CHECK(fixture.ids[2] == UINT64_C(8));
    check_request_fields(&fixture.requests[0], &expected);
    check_request_fields(&fixture.requests[1], &expected);
    check_request_fields(&fixture.requests[2], &expected);
}

static void test_fault_and_malformed_observation_preserve_pending_state(void)
{
    const endpoint_mode modes[] = {ENDPOINT_FAULT,
                                   ENDPOINT_INVALID_OBSERVATION,
                                   ENDPOINT_STALE_OBSERVATION};
    size_t index;

    for (index = 0U; index < sizeof(modes) / sizeof(modes[0]); ++index) {
        endpoint_fixture fixture = {
            .mode = modes[index],
            .read_value_word = UINT16_C(0x1111),
            .fault_status = TI_C54X_BUS_FAULT,
            .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
        const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
        ti_c54x_bus_word16_transaction transaction = prepare_transaction(
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT64_C(9));
        ti_c54x_bus_word16_adapter_result result =
            ti_c54x_bus_word16_adapter_advance(
                &endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);

        TI_C54X_TEST_CHECK(result.status ==
                           (modes[index] == ENDPOINT_FAULT
                                ? TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT
                                : TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_OBSERVATION));
        TI_C54X_TEST_CHECK(transaction.state ==
                           (modes[index] == ENDPOINT_FAULT
                                ? TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT
                                : TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT));
        TI_C54X_TEST_CHECK(fixture.callback_count == 1U);
        TI_C54X_TEST_CHECK(fixture.zeroed_observation_count == 1U);
    }
}

static void test_write_visibility_occurs_once_before_ready_returns(void)
{
    endpoint_fixture fixture = {
        .mode = ENDPOINT_WRITE_WAIT_THEN_READY,
        .read_value_word = UINT16_C(0),
        .fault_status = TI_C54X_BUS_FAULT,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_bus_word16_transaction transaction = prepare_transaction(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT64_C(10));
    ti_c54x_bus_word16_adapter_result result;

    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT);
    TI_C54X_TEST_CHECK(fixture.visible_write_count == 0U);
    TI_C54X_TEST_CHECK(fixture.zeroed_observation_count == 1U);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT);
    TI_C54X_TEST_CHECK(fixture.visible_write_count == 0U);
    TI_C54X_TEST_CHECK(fixture.zeroed_observation_count == 2U);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK);
    TI_C54X_TEST_CHECK(fixture.zeroed_observation_count == 3U);
    TI_C54X_TEST_CHECK(fixture.visible_write_count == 1U);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(fixture.visible_write_count == 1U);
}

static void test_cancel_failure_preserves_pending_transaction_for_retry(void)
{
    endpoint_fixture fixture = {
        .mode = ENDPOINT_WRITE_WAIT_THEN_READY,
        .read_value_word = UINT16_C(0),
        .fault_status = TI_C54X_BUS_FAULT,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_FAILED};
    const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_bus_word16_transaction transaction = prepare_transaction(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT64_C(12));
    ti_c54x_bus_word16_adapter_result result;

    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT);
    result = ti_c54x_bus_word16_adapter_cancel(&endpoint, &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_CANCEL_FAILED);
    TI_C54X_TEST_CHECK(fixture.cancel_count == 1U);
    TI_C54X_TEST_CHECK(fixture.visible_write_count == 0U);
    TI_C54X_TEST_CHECK(fixture.zeroed_observation_count == 1U);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT);

    fixture.cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
    result = ti_c54x_bus_word16_adapter_cancel(&endpoint, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK);
    TI_C54X_TEST_CHECK(fixture.cancel_count == 2U);
    TI_C54X_TEST_CHECK(fixture.visible_write_count == 0U);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE);
}

static void test_cancel_is_only_for_externally_active_inflight(void)
{
    endpoint_fixture fixture = {
        .mode = ENDPOINT_READ_READY,
        .read_value_word = UINT16_C(0x4567),
        .fault_status = TI_C54X_BUS_FAULT,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_bus_word16_transaction transaction = prepare_transaction(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT64_C(13));
    ti_c54x_bus_word16_adapter_result result;
    ti_c54x_bus_word16_observation ready = {
        UINT64_C(13), TI_C54X_BUS_WORD16_OBSERVATION_READY, UINT16_C(0x4567),
        TI_C54X_BUS_OK};

    result = ti_c54x_bus_word16_adapter_cancel(&endpoint, &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(fixture.cancel_count == 0U);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_ISSUE_READY);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_begin(&transaction).status ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(ti_c54x_bus_word16_transaction_poll(&transaction, &ready) ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATUS_OK);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETION_READY);
    result = ti_c54x_bus_word16_adapter_cancel(&endpoint, &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(fixture.cancel_count == 0U);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETION_READY);
}

static void test_forged_cancel_result_preserves_pending_transaction(void)
{
    endpoint_fixture fixture = {
        .mode = ENDPOINT_READ_WAIT_THEN_READY,
        .read_value_word = UINT16_C(0x1111),
        .fault_status = TI_C54X_BUS_FAULT,
        .cancel_result =
            (ti_c54x_bus_word16_cancel_result)UINT8_C(0x7f)};
    const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&fixture);
    ti_c54x_bus_word16_transaction transaction = prepare_transaction(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT64_C(14));
    ti_c54x_bus_word16_adapter_result result;

    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT);
    result = ti_c54x_bus_word16_adapter_cancel(&endpoint, &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_CANCEL_FAILED);
    TI_C54X_TEST_CHECK(fixture.cancel_count == 1U);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT);
}

int main(void)
{
    test_canonical_absent_and_partial_endpoints_are_rejected();
    test_immediate_read_ready_is_one_callback_and_one_completion();
    test_wait_resume_reuses_id_and_request_without_reissue();
    test_fault_and_malformed_observation_preserve_pending_state();
    test_write_visibility_occurs_once_before_ready_returns();
    test_cancel_failure_preserves_pending_transaction_for_retry();
    test_cancel_is_only_for_externally_active_inflight();
    test_forged_cancel_result_preserves_pending_transaction();
    return 0;
}
