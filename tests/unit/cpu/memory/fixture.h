/* Module contract
 * Owns: deterministic endpoint and observer fixture plumbing shared by the
 * CPU W31B1 memory lifecycle tests, including cancellation re-entry probes.
 * Does not own: endpoint/reducer implementation, memory semantics, opcode
 * identity, production state, or test discovery.
 */
#ifndef TI_C54X_TEST_CPU_MEMORY_FIXTURE_H
#define TI_C54X_TEST_CPU_MEMORY_FIXTURE_H

#include <ti_c54x/bus_endpoint.h>
#include <ti_c54x/cpu.h>

#include "../../../../src/cpu/cpu_internal.h"
#include "../../../support/test_harness.h"

#include <stdint.h>

typedef enum cpu_endpoint_mode {
    CPU_ENDPOINT_READY = 0,
    CPU_ENDPOINT_WAIT = 1,
    CPU_ENDPOINT_FAULT = 2
} cpu_endpoint_mode;

typedef struct cpu_memory_fixture {
    cpu_endpoint_mode mode;
    unsigned int advance_calls;
    unsigned int begin_calls;
    unsigned int resume_calls;
    unsigned int cancel_calls;
    unsigned int fetch_calls;
    unsigned int stop_events;
    unsigned int fetch_events;
    uint64_t ids[8];
    ti_c54x_bus_word16_request requests[8];
    uint16_t read_value_word;
    ti_c54x_bus_status fault_status;
    ti_c54x_bus_word16_cancel_result cancel_result;
    ti_c54x_cpu *reentry_cpu;
    ti_c54x_error reentry_status;
} cpu_memory_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    cpu_memory_fixture *fixture = (cpu_memory_fixture *)context;
    (void)address_word;
    TI_C54X_TEST_CHECK(value_word != NULL);
    fixture->fetch_calls++;
    *value_word = UINT16_C(0xf495);
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    cpu_memory_fixture *fixture = (cpu_memory_fixture *)context;
    const unsigned int index = fixture->advance_calls;

    TI_C54X_TEST_CHECK(index < sizeof(fixture->ids) / sizeof(fixture->ids[0]));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN ||
                       phase == TI_C54X_BUS_WORD16_ADVANCE_RESUME);
    fixture->ids[index] = transaction_id;
    fixture->requests[index] = *request;
    fixture->advance_calls++;
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        fixture->begin_calls++;
    } else {
        fixture->resume_calls++;
    }
    observation->transaction_id = transaction_id;
    observation->read_value_word = UINT16_C(0);
    if (fixture->mode == CPU_ENDPOINT_WAIT) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        observation->bus_status = TI_C54X_BUS_OK;
    } else if (fixture->mode == CPU_ENDPOINT_FAULT) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = fixture->fault_status;
    } else {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        observation->bus_status = TI_C54X_BUS_OK;
        if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
            observation->read_value_word = fixture->read_value_word;
        }
    }
}

static ti_c54x_bus_word16_cancel_result
cancel_data(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    cpu_memory_fixture *fixture = (cpu_memory_fixture *)context;
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    fixture->cancel_calls++;
    if (fixture->reentry_cpu != NULL) {
        fixture->reentry_status = ti_c54x_cpu_reset(fixture->reentry_cpu);
    }
    return fixture->cancel_result;
}

static void
emit_trace(void *context, const ti_c54x_trace_event *event)
{
    cpu_memory_fixture *fixture = (cpu_memory_fixture *)context;
    TI_C54X_TEST_CHECK(event != NULL);
    if (event->kind == TI_C54X_TRACE_FETCH) {
        fixture->fetch_events++;
    } else if (event->kind == TI_C54X_TRACE_STOP) {
        fixture->stop_events++;
    }
}

static ti_c54x_bus_word16_endpoint
make_endpoint(cpu_memory_fixture *fixture)
{
    return (ti_c54x_bus_word16_endpoint){
        .context = fixture, .advance = advance_data, .cancel = cancel_data};
}

static ti_c54x_bus
make_bus(cpu_memory_fixture *fixture,
         const ti_c54x_bus_word16_endpoint *endpoint)
{
    ti_c54x_bus bus = {0};
    bus.context = fixture;
    bus.program_read_word = read_program;
    if (endpoint != NULL) {
        bus.data_endpoint = *endpoint;
    }
    return bus;
}

static ti_c54x_cpu *
init_cpu(void *storage, cpu_memory_fixture *fixture,
         const ti_c54x_bus_word16_endpoint *endpoint,
         ti_c54x_trace_sink *trace, ti_c54x_error *status_out)
{
    const ti_c54x_bus bus = make_bus(fixture, endpoint);
    const ti_c54x_config config = {
        UINT32_C(0), trace != NULL ? *trace : (ti_c54x_trace_sink){0}};
    ti_c54x_cpu *cpu = NULL;

    *status_out = ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &bus,
                                   &cpu);
    return cpu;
}

#endif
