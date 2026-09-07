/*
 * Module contract
 *
 * Owns: executable PROJECT_CONTRACT_ONLY proof that the WORD16 adapter
 * authorizes one BEGIN, resumes an existing WAIT transaction, and exposes a
 * READY write exactly once.
 * Does not own: public endpoint declarations, physical-memory implementation,
 * byte order, arbitration, device timing, CPU lifecycle, opcode semantics,
 * diagnostics, or test discovery outside this owner.
 */

#include "../../../../src/bus/adapter/word16_adapter.h"
#include "../../../../src/bus/request/word16_request.h"
#include "../../../../src/bus/transaction/word16_transaction.h"
#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>

typedef enum scripted_response {
    SCRIPT_WAIT = 0,
    SCRIPT_READY = 1,
    SCRIPT_FAULT = 2,
    SCRIPT_MALFORMED_ID = 3,
    SCRIPT_MALFORMED_KIND = 4,
    SCRIPT_WAIT_WITH_READ = 5,
    SCRIPT_READY_WITH_FAULT = 6,
    SCRIPT_FAULT_WITH_OK = 7,
    SCRIPT_FAULT_WITH_READ = 8,
    SCRIPT_WRITE_READY_WITH_READ = 9
} scripted_response;

typedef struct scripted_endpoint {
    scripted_response responses[4];
    size_t response_count;
    size_t callback_count;
    size_t cancel_count;
    size_t visible_write_count;
    size_t zeroed_observation_count;
    uint64_t ids[4];
    ti_c54x_bus_word16_request requests[4];
    ti_c54x_bus_word16_cancel_result cancel_result;
} scripted_endpoint;

static ti_c54x_bus_word16_request make_request(
    ti_c54x_bus_word16_request_access access)
{
    const ti_c54x_bus_word16_request_result result =
        access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
            ? ti_c54x_bus_word16_request_read(
                  TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, UINT32_C(0x0042))
            : ti_c54x_bus_word16_request_write(
                  TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA, UINT32_C(0x0042),
                  UINT16_C(0x5aa5));

    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_REQUEST_STATUS_OK);
    return result.request;
}

static ti_c54x_bus_word16_transaction make_transaction(
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

static void scripted_advance(void *context,
                             ti_c54x_bus_word16_advance_phase phase,
                             uint64_t transaction_id,
                             const ti_c54x_bus_word16_request *request,
                             ti_c54x_bus_word16_observation *observation)
{
    scripted_endpoint *script = (scripted_endpoint *)context;
    const size_t index = script->callback_count;
    const scripted_response response =
        index < script->response_count ? script->responses[index]
                                       : SCRIPT_READY;

    TI_C54X_TEST_CHECK(index < sizeof(script->ids) / sizeof(script->ids[0]));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN ||
                       phase == TI_C54X_BUS_WORD16_ADVANCE_RESUME);
    TI_C54X_TEST_CHECK(observation != NULL);
    if (observation->transaction_id == UINT64_C(0) &&
        observation->kind == TI_C54X_BUS_WORD16_OBSERVATION_INVALID &&
        observation->read_value_word == UINT16_C(0) &&
        observation->bus_status == TI_C54X_BUS_OK) {
        script->zeroed_observation_count += 1U;
    }
    script->ids[index] = transaction_id;
    script->requests[index] = *request;
    script->callback_count += 1U;
    observation->transaction_id = transaction_id;
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
    observation->read_value_word = UINT16_C(0);
    observation->bus_status = TI_C54X_BUS_OK;

    switch (response) {
    case SCRIPT_WAIT:
        break;
    case SCRIPT_READY:
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        observation->read_value_word = UINT16_C(0xcafe);
        if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
            observation->read_value_word = UINT16_C(0);
            script->visible_write_count += 1U;
        }
        break;
    case SCRIPT_FAULT:
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        break;
    case SCRIPT_MALFORMED_ID:
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        observation->transaction_id = transaction_id + UINT64_C(1);
        observation->read_value_word = UINT16_C(0xcafe);
        break;
    case SCRIPT_MALFORMED_KIND:
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_INVALID;
        break;
    case SCRIPT_WAIT_WITH_READ:
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        observation->read_value_word = UINT16_C(1);
        break;
    case SCRIPT_READY_WITH_FAULT:
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        observation->bus_status = TI_C54X_BUS_FAULT;
        break;
    case SCRIPT_FAULT_WITH_OK:
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_OK;
        break;
    case SCRIPT_FAULT_WITH_READ:
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->read_value_word = UINT16_C(1);
        observation->bus_status = TI_C54X_BUS_FAULT;
        break;
    case SCRIPT_WRITE_READY_WITH_READ:
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        observation->read_value_word = UINT16_C(1);
        break;
    default:
        TI_C54X_TEST_CHECK(false);
        break;
    }
}

static ti_c54x_bus_word16_cancel_result scripted_cancel(
    void *context,
    uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request)
{
    scripted_endpoint *script = (scripted_endpoint *)context;

    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    script->cancel_count += 1U;
    return script->cancel_result;
}

static ti_c54x_bus_word16_endpoint make_endpoint(scripted_endpoint *script)
{
    ti_c54x_bus_word16_endpoint endpoint = {0};

    endpoint.context = script;
    endpoint.advance = scripted_advance;
    endpoint.cancel = scripted_cancel;
    return endpoint;
}

static void test_read_wait_then_ready_has_one_begin_and_no_refetch(void)
{
    scripted_endpoint script = {
        .responses = {SCRIPT_WAIT, SCRIPT_WAIT, SCRIPT_READY},
        .response_count = 3U,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&script);
    ti_c54x_bus_word16_transaction transaction = make_transaction(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ, UINT64_C(11));
    const ti_c54x_bus_word16_request expected = transaction.request;
    ti_c54x_bus_word16_adapter_result result;

    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK);
    TI_C54X_TEST_CHECK(result.read_value_word == UINT16_C(0xcafe));
    TI_C54X_TEST_CHECK(script.callback_count == 3U);
    TI_C54X_TEST_CHECK(script.zeroed_observation_count == 3U);
    TI_C54X_TEST_CHECK(script.ids[0] == UINT64_C(11));
    TI_C54X_TEST_CHECK(script.ids[1] == UINT64_C(11));
    TI_C54X_TEST_CHECK(script.ids[2] == UINT64_C(11));
    TI_C54X_TEST_CHECK(script.requests[0].address_word ==
                       expected.address_word);
    TI_C54X_TEST_CHECK(script.requests[1].address_word ==
                       expected.address_word);
    TI_C54X_TEST_CHECK(script.requests[2].address_word ==
                       expected.address_word);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_COMPLETE);
}

static void test_write_is_not_visible_until_ready_and_never_twice(void)
{
    scripted_endpoint script = {
        .responses = {SCRIPT_WAIT, SCRIPT_FAULT},
        .response_count = 2U,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&script);
    ti_c54x_bus_word16_transaction transaction = make_transaction(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT64_C(12));
    ti_c54x_bus_word16_adapter_result result;

    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.zeroed_observation_count == 1U);
    TI_C54X_TEST_CHECK(script.visible_write_count == 0U);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_FAULT);
    TI_C54X_TEST_CHECK(script.visible_write_count == 0U);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_FAULT);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(script.callback_count == 2U);
    TI_C54X_TEST_CHECK(script.zeroed_observation_count == 2U);
    TI_C54X_TEST_CHECK(script.visible_write_count == 0U);

    script = (scripted_endpoint){
        .responses = {SCRIPT_WAIT, SCRIPT_READY},
        .response_count = 2U,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    transaction = make_transaction(TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
                                   UINT64_C(13));
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.visible_write_count == 0U);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK);
    TI_C54X_TEST_CHECK(script.visible_write_count == 1U);
    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_RESUME, &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(script.callback_count == 2U);
    TI_C54X_TEST_CHECK(script.zeroed_observation_count == 2U);
    TI_C54X_TEST_CHECK(script.visible_write_count == 1U);
}

static void test_cancel_prevents_publication_and_rejects_terminal_retries(void)
{
    scripted_endpoint script = {
        .responses = {SCRIPT_WAIT},
        .response_count = 1U,
        .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
    const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&script);
    ti_c54x_bus_word16_transaction transaction = make_transaction(
        TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE, UINT64_C(14));
    ti_c54x_bus_word16_adapter_result result;

    result = ti_c54x_bus_word16_adapter_advance(
        &endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_WAIT);
    TI_C54X_TEST_CHECK(script.zeroed_observation_count == 1U);
    TI_C54X_TEST_CHECK(script.visible_write_count == 0U);
    result = ti_c54x_bus_word16_adapter_cancel(&endpoint, &transaction);
    TI_C54X_TEST_CHECK(result.status == TI_C54X_BUS_WORD16_ADAPTER_STATUS_OK);
    TI_C54X_TEST_CHECK(script.cancel_count == 1U);
    TI_C54X_TEST_CHECK(script.visible_write_count == 0U);
    TI_C54X_TEST_CHECK(transaction.state ==
                       TI_C54X_BUS_WORD16_TRANSACTION_STATE_IDLE);
    result = ti_c54x_bus_word16_adapter_cancel(&endpoint, &transaction);
    TI_C54X_TEST_CHECK(result.status ==
                       TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(script.cancel_count == 1U);
    TI_C54X_TEST_CHECK(script.callback_count == 1U);
}

static void test_malformed_id_or_kind_does_not_publish_or_reissue(void)
{
    const scripted_response malformed[] = {
        SCRIPT_MALFORMED_ID, SCRIPT_MALFORMED_KIND, SCRIPT_WAIT_WITH_READ,
        SCRIPT_READY_WITH_FAULT, SCRIPT_FAULT_WITH_OK, SCRIPT_FAULT_WITH_READ,
        SCRIPT_WRITE_READY_WITH_READ};
    size_t index;

    for (index = 0U; index < sizeof(malformed) / sizeof(malformed[0]); ++index) {
        scripted_endpoint script = {
            .responses = {malformed[index]},
            .response_count = 1U,
            .cancel_result = TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED};
        const ti_c54x_bus_word16_endpoint endpoint = make_endpoint(&script);
        ti_c54x_bus_word16_transaction transaction = make_transaction(
            TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE,
            UINT64_C(15) + (uint64_t)index);
        ti_c54x_bus_word16_adapter_result result =
            ti_c54x_bus_word16_adapter_advance(
                &endpoint, TI_C54X_BUS_WORD16_ADVANCE_BEGIN, &transaction);

        TI_C54X_TEST_CHECK(result.status ==
                           TI_C54X_BUS_WORD16_ADAPTER_STATUS_INVALID_OBSERVATION);
        TI_C54X_TEST_CHECK(transaction.state ==
                           TI_C54X_BUS_WORD16_TRANSACTION_STATE_IN_FLIGHT);
        TI_C54X_TEST_CHECK(script.callback_count == 1U);
        TI_C54X_TEST_CHECK(script.zeroed_observation_count == 1U);
        TI_C54X_TEST_CHECK(script.visible_write_count == 0U);
    }
}

int main(void)
{
    test_read_wait_then_ready_has_one_begin_and_no_refetch();
    test_write_is_not_visible_until_ready_and_never_twice();
    test_cancel_prevents_publication_and_rejects_terminal_retries();
    test_malformed_id_or_kind_does_not_publish_or_reissue();
    return 0;
}
