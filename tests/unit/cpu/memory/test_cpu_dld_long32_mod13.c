/* Module contract
 * Owns: CPU-facing READY/WAIT/FAULT proofs for DLD LONG32 MOD13, including
 * two-word program fetch, paired data requests, deferred AR/PC publication,
 * SXM knownness, C16 independence, and paired transaction identity.
 * Does not own: production semantics, catalog/decode implementation, bus
 * adapters, benchmarks, documentation, or shared fixture implementation.
 */

#include "fixture.h"

#include "../../../../src/state/register_layout.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum dld_mode {
    DLD_READY = 0,
    DLD_WAIT_FIRST,
    DLD_WAIT_SECOND,
    DLD_FAULT_FIRST,
    DLD_FAULT_SECOND
} dld_mode;

typedef struct dld_fixture {
    cpu_memory_fixture common;
    dld_mode mode;
    uint16_t program_words[2];
    uint16_t first_address;
    uint16_t read_msw;
    uint16_t read_lsw;
    ti_c54x_bus_status fault_status;
    unsigned program_reads;
    unsigned endpoint_calls;
    unsigned begin_calls;
    unsigned resume_calls;
    unsigned ready_calls;
    unsigned fetch_events;
    unsigned stop_events;
    uint64_t ids[8];
    ti_c54x_bus_word16_request requests[8];
} dld_fixture;

static ti_c54x_bus_status dld_read_program(
    void *context, ti_c54x_program_word_address address_word,
    uint16_t *value_word)
{
    dld_fixture *fixture = (dld_fixture *)context;

    TI_C54X_TEST_CHECK(value_word != NULL);
    TI_C54X_TEST_CHECK(fixture->program_reads < 2U);
    TI_C54X_TEST_CHECK(address_word == fixture->program_reads);
    *value_word = fixture->program_words[fixture->program_reads++];
    return TI_C54X_BUS_OK;
}

static void dld_advance_data(
    void *context, ti_c54x_bus_word16_advance_phase phase,
    uint64_t transaction_id, const ti_c54x_bus_word16_request *request,
    ti_c54x_bus_word16_observation *observation)
{
    dld_fixture *fixture = (dld_fixture *)context;
    const unsigned index = fixture->endpoint_calls++;
    bool first;

    TI_C54X_TEST_CHECK(index < sizeof(fixture->ids) / sizeof(fixture->ids[0]));
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN ||
                       phase == TI_C54X_BUS_WORD16_ADVANCE_RESUME);
    first = request->address_word == fixture->first_address;
    fixture->ids[index] = transaction_id;
    fixture->requests[index] = *request;
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        fixture->begin_calls++;
    } else {
        fixture->resume_calls++;
    }
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY,
        .bus_status = TI_C54X_BUS_OK,
        .read_value_word = first ? fixture->read_msw : fixture->read_lsw};
    if ((fixture->mode == DLD_WAIT_FIRST && index == 0U) ||
        (fixture->mode == DLD_WAIT_SECOND && index == 1U)) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        observation->read_value_word = UINT16_C(0);
    }
    if ((fixture->mode == DLD_FAULT_FIRST && index == 0U) ||
        (fixture->mode == DLD_FAULT_SECOND && index == 1U)) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = fixture->fault_status;
        observation->read_value_word = UINT16_C(0);
    }
    if (observation->kind == TI_C54X_BUS_WORD16_OBSERVATION_READY) {
        fixture->ready_calls++;
    }
}

static ti_c54x_bus_word16_cancel_result dld_cancel_data(
    void *context, uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request)
{
    dld_fixture *fixture = (dld_fixture *)context;
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    fixture->common.cancel_calls++;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static void dld_emit_trace(void *context, const ti_c54x_trace_event *event)
{
    dld_fixture *fixture = (dld_fixture *)context;
    TI_C54X_TEST_CHECK(event != NULL);
    if (event->kind == TI_C54X_TRACE_FETCH) {
        fixture->fetch_events++;
    } else if (event->kind == TI_C54X_TRACE_STOP) {
        fixture->stop_events++;
    }
}

static ti_c54x_cpu *dld_init(dld_fixture *fixture, void *storage,
                             uint8_t destination, uint8_t arf,
                             uint16_t extension, uint16_t ar_value,
                             ti_c54x_error *status)
{
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_bus bus;
    ti_c54x_trace_sink trace;
    ti_c54x_config config;
    ti_c54x_cpu *cpu = NULL;

    /* The shared fixture also exposes the ordinary one-word initializer and
     * sink; retain those helpers as part of the common fixture contract even
     * though this test supplies a two-word program callback. */
    (void)init_cpu;
    (void)emit_trace;

    fixture->program_words[0] = (uint16_t)(UINT16_C(0x56E8) |
                                           ((uint16_t)(destination & 1U) << 8U) |
                                           (uint16_t)(arf & 7U));
    fixture->program_words[1] = extension;
    fixture->first_address = (uint16_t)(ar_value + extension);

    /* Route construction through the shared fixture's copied-descriptor
     * helpers; this test then supplies its two-word program and pair-aware
     * callbacks below. */
    endpoint = make_endpoint(&fixture->common);
    bus = make_bus(&fixture->common, &endpoint);
    endpoint.context = fixture;
    endpoint.advance = dld_advance_data;
    endpoint.cancel = dld_cancel_data;
    bus.context = fixture;
    bus.program_read_word = dld_read_program;
    bus.data_endpoint = endpoint;
    trace = (ti_c54x_trace_sink){TI_C54X_TRACE_MASK_ALL, fixture,
                                 dld_emit_trace};
    config = (ti_c54x_config){UINT32_C(0), trace};
    *status = ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &bus,
                               &cpu);
    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state, arf,
                                                   ar_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x1111222233)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000AABBCC)) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void dld_set_sxm(ti_c54x_cpu *cpu, uint8_t sxm, int known)
{
    TI_C54X_TEST_CHECK(sxm <= 1U);
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 &
                   (uint16_t)~TI_C54X_ST1_SXM_MASK);
    if (sxm != 0U) {
        cpu->arch_state.status.st1 =
            (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_SXM_MASK);
    }
    if (known != 0) {
        cpu->arch_state.status.st1_known_mask =
            (uint16_t)(cpu->arch_state.status.st1_known_mask |
                       TI_C54X_ST1_SXM_MASK);
    } else {
        cpu->arch_state.status.st1_known_mask =
            (uint16_t)(cpu->arch_state.status.st1_known_mask &
                       (uint16_t)~TI_C54X_ST1_SXM_MASK);
    }
}

static void dld_set_c16(ti_c54x_cpu *cpu, uint8_t c16, int known)
{
    TI_C54X_TEST_CHECK(c16 <= 1U);
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 &
                   (uint16_t)~TI_C54X_ST1_C16_MASK);
    if (c16 != 0U) {
        cpu->arch_state.status.st1 =
            (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_C16_MASK);
    }
    if (known != 0) {
        cpu->arch_state.status.st1_known_mask =
            (uint16_t)(cpu->arch_state.status.st1_known_mask |
                       TI_C54X_ST1_C16_MASK);
    } else {
        cpu->arch_state.status.st1_known_mask =
            (uint16_t)(cpu->arch_state.status.st1_known_mask &
                       (uint16_t)~TI_C54X_ST1_C16_MASK);
    }
}

static void dld_check_ready(dld_fixture *fixture, ti_c54x_cpu *cpu,
                            uint8_t arf, uint16_t ar_before,
                            uint16_t extension, uint8_t destination,
                            uint8_t sxm)
{
    const uint16_t first = (uint16_t)(ar_before + extension);
    const uint16_t second = (first & 1U) == 0U ? (uint16_t)(first + 1U)
                                               : (uint16_t)(first - 1U);
    const uint64_t expected = ((uint64_t)fixture->read_msw << 16U) |
                              fixture->read_lsw |
                              (sxm != 0U &&
                               (fixture->read_msw & UINT16_C(0x8000)) != 0U
                                   ? UINT64_C(0xFF00000000)
                                   : UINT64_C(0));
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(result.opcode_word == fixture->program_words[0]);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture->program_reads == 2U);
    TI_C54X_TEST_CHECK(fixture->endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture->begin_calls == 2U);
    TI_C54X_TEST_CHECK(fixture->resume_calls == 0U);
    TI_C54X_TEST_CHECK(fixture->ready_calls == 2U);
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word == first);
    TI_C54X_TEST_CHECK(fixture->requests[1].address_word == second);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture->requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture->ids[0] != UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture->ids[1] == fixture->ids[0] + UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == first);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(destination == 0U ? cpu->accumulators.a_raw40 == expected
                                         : cpu->accumulators.b_raw40 == expected);
    TI_C54X_TEST_CHECK(destination == 0U ?
                           (cpu->accumulators.known_mask & 1U) != 0U :
                           (cpu->accumulators.known_mask & 2U) != 0U);
}

static void test_ready_destination_sxm_and_c16_forms(void)
{
    for (uint8_t destination = 0U; destination <= 1U; ++destination) {
        for (uint8_t sxm = 0U; sxm <= 1U; ++sxm) {
            for (uint8_t c16 = 0U; c16 <= 1U; ++c16) {
                dld_fixture fixture = {
                    .mode = DLD_READY,
                    .read_msw = UINT16_C(0x8001),
                    .read_lsw = UINT16_C(0x2233),
                    .fault_status = TI_C54X_BUS_FAULT};
                _Alignas(max_align_t) unsigned char storage[1024] = {0};
                ti_c54x_error status;
                const uint16_t ar_before =
                    c16 == 0U ? UINT16_C(0x2100) : UINT16_C(0x2101);
                ti_c54x_cpu *cpu = dld_init(
                    &fixture, storage, destination, UINT8_C(3), UINT16_C(2),
                    ar_before, &status);

                dld_set_sxm(cpu, sxm, 1);
                dld_set_c16(cpu, c16, 1);
                dld_check_ready(&fixture, cpu, UINT8_C(3), ar_before,
                                UINT16_C(2), destination, sxm);
                TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                                   TI_C54X_ERROR_OK);
            }
        }
    }
}

static void test_unknown_c16_is_ignored(void)
{
    dld_fixture fixture = {.mode = DLD_READY,
                            .read_msw = UINT16_C(0x7FFF),
                            .read_lsw = UINT16_C(0xFFFE),
                            .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = dld_init(&fixture, storage, 0U, 2U, 2U, 0x2300U,
                                &status);
    dld_set_sxm(cpu, 0U, 1);
    dld_set_c16(cpu, 1U, 0);
    dld_check_ready(&fixture, cpu, 2U, 0x2300U, 2U, 0U, 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void test_unknown_sxm_rejects_before_endpoint(void)
{
    dld_fixture fixture = {.mode = DLD_READY,
                            .read_msw = UINT16_C(0x1234),
                            .read_lsw = UINT16_C(0x5678),
                            .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = dld_init(&fixture, storage, 1U, 4U, 2U, 0x2400U,
                                &status);
    dld_set_sxm(cpu, 0U, 0);
    {
        const ti_c54x_arch_state before = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators = cpu->accumulators;
        TI_C54X_TEST_CHECK(ti_c54x_cpu_run(cpu, UINT64_C(1)).reason ==
                           TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(fixture.program_reads == 2U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before,
                                  sizeof(before)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void test_wait_first_and_second_resume_without_refetch(void)
{
    for (dld_mode mode = DLD_WAIT_FIRST; mode <= DLD_WAIT_SECOND; ++mode) {
        dld_fixture fixture = {.mode = mode,
                                .read_msw = UINT16_C(0x8000),
                                .read_lsw = UINT16_C(0x0001),
                                .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = dld_init(&fixture, storage, 0U, 5U, 2U, 0x2500U,
                                    &status);
        const uint16_t first = UINT16_C(0x2502);
        const uint16_t second = UINT16_C(0x2503);
        const ti_c54x_run_result wait = ti_c54x_cpu_run(cpu, UINT64_C(1));

        TI_C54X_TEST_CHECK(wait.reason == TI_C54X_STOP_BUS_WAIT);
        TI_C54X_TEST_CHECK(wait.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(wait.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.program_reads == 2U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[5] == UINT16_C(0x2500));
        TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 == UINT64_C(0x1111222233));
        fixture.mode = DLD_READY;
        {
            const ti_c54x_run_result ready = ti_c54x_cpu_run(cpu, UINT64_C(1));
            TI_C54X_TEST_CHECK(ready.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
            TI_C54X_TEST_CHECK(ready.instructions_attempted == UINT64_C(0));
            TI_C54X_TEST_CHECK(ready.instructions_retired == UINT64_C(1));
            TI_C54X_TEST_CHECK(fixture.program_reads == 2U);
            TI_C54X_TEST_CHECK(fixture.endpoint_calls == 3U);
            TI_C54X_TEST_CHECK(fixture.begin_calls == 2U);
            TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
            TI_C54X_TEST_CHECK(fixture.ids[0] != UINT64_C(0));
            if (mode == DLD_WAIT_FIRST) {
                TI_C54X_TEST_CHECK(fixture.ids[1] == fixture.ids[0]);
                TI_C54X_TEST_CHECK(fixture.ids[2] ==
                                   fixture.ids[0] + UINT64_C(1));
                TI_C54X_TEST_CHECK(fixture.requests[0].address_word == first);
                TI_C54X_TEST_CHECK(fixture.requests[1].address_word == first);
                TI_C54X_TEST_CHECK(fixture.requests[2].address_word == second);
            } else {
                TI_C54X_TEST_CHECK(fixture.ids[1] == fixture.ids[0] + UINT64_C(1));
                TI_C54X_TEST_CHECK(fixture.ids[2] == fixture.ids[1]);
                TI_C54X_TEST_CHECK(fixture.requests[0].address_word == first);
                TI_C54X_TEST_CHECK(fixture.requests[1].address_word == second);
                TI_C54X_TEST_CHECK(fixture.requests[2].address_word == second);
            }
            TI_C54X_TEST_CHECK(cpu->arch_state.ar[5] == first);
            TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(2));
            TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 == UINT64_C(0xFF80000001));
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void test_fault_first_and_second_never_publish_dld(void)
{
    for (dld_mode mode = DLD_FAULT_FIRST; mode <= DLD_FAULT_SECOND; ++mode) {
        dld_fixture fixture = {.mode = mode,
                                .read_msw = UINT16_C(0xFFFF),
                                .read_lsw = UINT16_C(0xFFFF),
                                .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = dld_init(&fixture, storage, 1U, 6U, 2U, 0x2600U,
                                    &status);
        const uint64_t before_b = cpu->accumulators.b_raw40;
        const ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[6] == UINT16_C(0x2600));
        TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 == before_b);
        TI_C54X_TEST_CHECK(fixture.program_reads == 2U);
        TI_C54X_TEST_CHECK(fixture.ready_calls ==
                           (mode == DLD_FAULT_SECOND ? 1U : 0U));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls ==
                           (mode == DLD_FAULT_SECOND ? 2U : 1U));
        if (mode == DLD_FAULT_SECOND) {
            TI_C54X_TEST_CHECK(fixture.ids[1] == fixture.ids[0] + UINT64_C(1));
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

int main(void)
{
    test_ready_destination_sxm_and_c16_forms();
    test_unknown_c16_is_ignored();
    test_unknown_sxm_rejects_before_endpoint();
    test_wait_first_and_second_resume_without_refetch();
    test_fault_first_and_second_never_publish_dld();
    return 0;
}
