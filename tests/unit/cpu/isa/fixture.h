/* Module contract
 * Owns: public-CPU end-to-end fixtures for the fixed-MOD2 WORD16 memory
 * instruction RED tests, including program fetch, endpoint observations,
 * cancellation, and trace counts.
 * Does not own: production memory semantics, opcode decoding, CPU lifecycle,
 * architectural policy, or test registration.
 */
#ifndef TI_C54X_TEST_CPU_ISA_MEMORY_FIXTURE_H
#define TI_C54X_TEST_CPU_ISA_MEMORY_FIXTURE_H

#include <ti_c54x/bus_endpoint.h>
#include <ti_c54x/cpu.h>

#include "../../../../src/cpu/cpu_internal.h"
#include "../../../../src/state/register_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum ti_c54x_cpu_isa_memory_mode {
    TI_C54X_CPU_ISA_MEMORY_READY = 0,
    TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY = 1,
    TI_C54X_CPU_ISA_MEMORY_FAULT = 2,
    TI_C54X_CPU_ISA_MEMORY_WAIT_ALWAYS = 3
} ti_c54x_cpu_isa_memory_mode;

typedef struct ti_c54x_cpu_isa_memory_fixture {
    ti_c54x_cpu_isa_memory_mode mode;
    uint16_t program_word;
    /* Second program word for two-word forms (MVDM absolute);
     * served only when program_has_extension is true. */
    uint16_t program_extension_word;
    bool program_has_extension;
    uint16_t read_value_word;
    ti_c54x_bus_status fault_status;
    unsigned program_reads;
    unsigned endpoint_calls;
    unsigned begin_calls;
    unsigned resume_calls;
    unsigned ready_calls;
    unsigned cancel_calls;
    unsigned fetch_events;
    unsigned stop_events;
    uint64_t ids[4];
    ti_c54x_bus_word16_request requests[4];
} ti_c54x_cpu_isa_memory_fixture;

static ti_c54x_bus_status
ti_c54x_cpu_isa_read_program(void *context,
                             ti_c54x_program_word_address address_word,
                             uint16_t *value_word)
{
    ti_c54x_cpu_isa_memory_fixture *fixture =
        (ti_c54x_cpu_isa_memory_fixture *)context;

    TI_C54X_TEST_CHECK(value_word != NULL);
    if (address_word == UINT32_C(0)) {
        TI_C54X_TEST_CHECK(fixture->program_reads == 0U);
        fixture->program_reads++;
        *value_word = fixture->program_word;
        return TI_C54X_BUS_OK;
    }
    TI_C54X_TEST_CHECK(address_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture->program_has_extension);
    TI_C54X_TEST_CHECK(fixture->program_reads == 1U);
    fixture->program_reads++;
    *value_word = fixture->program_extension_word;
    return TI_C54X_BUS_OK;
}

static void
ti_c54x_cpu_isa_advance_data(void *context,
                             ti_c54x_bus_word16_advance_phase phase,
                             uint64_t transaction_id,
                             const ti_c54x_bus_word16_request *request,
                             ti_c54x_bus_word16_observation *observation)
{
    ti_c54x_cpu_isa_memory_fixture *fixture =
        (ti_c54x_cpu_isa_memory_fixture *)context;
    const unsigned index = fixture->endpoint_calls;

    TI_C54X_TEST_CHECK(index < sizeof(fixture->ids) / sizeof(fixture->ids[0]));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    fixture->ids[index] = transaction_id;
    fixture->requests[index] = *request;
    fixture->endpoint_calls++;
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        fixture->begin_calls++;
    } else {
        fixture->resume_calls++;
    }
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .bus_status = TI_C54X_BUS_OK,
        .read_value_word = UINT16_C(0)};
    if (fixture->mode == TI_C54X_CPU_ISA_MEMORY_FAULT) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = fixture->fault_status;
    } else if (fixture->mode == TI_C54X_CPU_ISA_MEMORY_WAIT_ALWAYS ||
               (fixture->mode == TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY &&
                fixture->endpoint_calls == 1U)) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
    } else {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        fixture->ready_calls++;
        if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
            observation->read_value_word = fixture->read_value_word;
        }
    }
}

static ti_c54x_bus_word16_cancel_result
ti_c54x_cpu_isa_cancel_data(void *context, uint64_t transaction_id,
                            const ti_c54x_bus_word16_request *request)
{
    ti_c54x_cpu_isa_memory_fixture *fixture =
        (ti_c54x_cpu_isa_memory_fixture *)context;

    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    fixture->cancel_calls++;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_bus_word16_endpoint
ti_c54x_cpu_isa_endpoint(ti_c54x_cpu_isa_memory_fixture *fixture)
{
    return (ti_c54x_bus_word16_endpoint){
        .context = fixture,
        .advance = ti_c54x_cpu_isa_advance_data,
        .cancel = ti_c54x_cpu_isa_cancel_data};
}

static ti_c54x_bus
ti_c54x_cpu_isa_bus(ti_c54x_cpu_isa_memory_fixture *fixture,
                    bool with_endpoint)
{
    ti_c54x_bus bus = {0};
    bus.context = fixture;
    bus.program_read_word = ti_c54x_cpu_isa_read_program;
    if (with_endpoint) {
        bus.data_endpoint = ti_c54x_cpu_isa_endpoint(fixture);
    }
    return bus;
}

static ti_c54x_cpu *
ti_c54x_cpu_isa_init(void *storage,
                     ti_c54x_cpu_isa_memory_fixture *fixture,
                     bool with_endpoint, ti_c54x_error *status_out)
{
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};
    const ti_c54x_bus bus = ti_c54x_cpu_isa_bus(fixture, with_endpoint);
    ti_c54x_cpu *cpu = NULL;

    *status_out = ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &bus,
                                   &cpu);
    return cpu;
}

static void
ti_c54x_cpu_isa_seed_address(ti_c54x_cpu *cpu, uint8_t arf,
                             uint16_t address_word)
{
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(
                           &cpu->arch_state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(
                           &cpu->arch_state, UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &cpu->arch_state, arf, address_word) ==
                       TI_C54X_ARCH_STATE_OK);
}

static inline void
ti_c54x_cpu_isa_seed_sxm(ti_c54x_cpu *cpu, uint8_t sxm)
{
    TI_C54X_TEST_CHECK(sxm <= UINT8_C(1));
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 &
                   (uint16_t)~TI_C54X_ST1_SXM_MASK);
    if (sxm != UINT8_C(0)) {
        cpu->arch_state.status.st1 =
            (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_SXM_MASK);
    }
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_SXM_MASK);
}

static inline void
ti_c54x_cpu_isa_emit_trace(void *context, const ti_c54x_trace_event *event)
{
    ti_c54x_cpu_isa_memory_fixture *fixture =
        (ti_c54x_cpu_isa_memory_fixture *)context;

    TI_C54X_TEST_CHECK(event != NULL);
    if (event->kind == TI_C54X_TRACE_FETCH) {
        fixture->fetch_events++;
    } else if (event->kind == TI_C54X_TRACE_STOP) {
        fixture->stop_events++;
    }
}

#endif
