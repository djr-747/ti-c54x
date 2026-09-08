/* Module contract
 * Owns: CPU-facing READY/WAIT/FAULT proofs for DST LONG32 MOD13, including
 * copied full-40-bit source values, SST/SXM saturation, paired MSW/LSW writes,
 * deferred AR/PC publication, and visible first-write fault behavior.
 * Does not own: production semantics, catalog/decode implementation, bus
 * adapters, benchmarks, documentation, or shared fixture implementation.
 */

#include "fixture.h"

#include "../../../../src/state/register_layout.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum dst_mode {
    DST_READY = 0,
    DST_WAIT_FIRST,
    DST_WAIT_SECOND,
    DST_FAULT_FIRST,
    DST_FAULT_SECOND
} dst_mode;

typedef struct dst_fixture {
    cpu_memory_fixture common;
    dst_mode mode;
    uint16_t program_words[2];
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
    uint16_t visible_writes[8];
} dst_fixture;

static ti_c54x_bus_status dst_read_program(
    void *context, ti_c54x_program_word_address address_word,
    uint16_t *value_word)
{
    dst_fixture *fixture = (dst_fixture *)context;

    TI_C54X_TEST_CHECK(value_word != NULL);
    TI_C54X_TEST_CHECK(fixture->program_reads < 2U);
    TI_C54X_TEST_CHECK(address_word == fixture->program_reads);
    *value_word = fixture->program_words[fixture->program_reads++];
    return TI_C54X_BUS_OK;
}

static void dst_advance_data(
    void *context, ti_c54x_bus_word16_advance_phase phase,
    uint64_t transaction_id, const ti_c54x_bus_word16_request *request,
    ti_c54x_bus_word16_observation *observation)
{
    dst_fixture *fixture = (dst_fixture *)context;
    const unsigned index = fixture->endpoint_calls++;

    TI_C54X_TEST_CHECK(index < sizeof(fixture->ids) / sizeof(fixture->ids[0]));
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN ||
                       phase == TI_C54X_BUS_WORD16_ADVANCE_RESUME);
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
        .read_value_word = UINT16_C(0)};
    if ((fixture->mode == DST_WAIT_FIRST && index == 0U) ||
        (fixture->mode == DST_WAIT_SECOND && index == 1U)) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
    }
    if ((fixture->mode == DST_FAULT_FIRST && index == 0U) ||
        (fixture->mode == DST_FAULT_SECOND && index == 1U)) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = fixture->fault_status;
    }
    if (observation->kind == TI_C54X_BUS_WORD16_OBSERVATION_READY) {
        fixture->visible_writes[index] = request->write_value_word;
        fixture->ready_calls++;
    }
}

static ti_c54x_bus_word16_cancel_result dst_cancel_data(
    void *context, uint64_t transaction_id,
    const ti_c54x_bus_word16_request *request)
{
    dst_fixture *fixture = (dst_fixture *)context;
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    fixture->common.cancel_calls++;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static void dst_emit_trace(void *context, const ti_c54x_trace_event *event)
{
    dst_fixture *fixture = (dst_fixture *)context;
    TI_C54X_TEST_CHECK(event != NULL);
    if (event->kind == TI_C54X_TRACE_FETCH) {
        fixture->fetch_events++;
    } else if (event->kind == TI_C54X_TRACE_STOP) {
        fixture->stop_events++;
    }
}

static ti_c54x_cpu *dst_init(dst_fixture *fixture, void *storage,
                             uint8_t source, uint8_t arf, uint16_t extension,
                             uint16_t ar_value, uint64_t source_value,
                             ti_c54x_error *status)
{
    ti_c54x_bus_word16_endpoint endpoint;
    ti_c54x_bus bus;
    ti_c54x_trace_sink trace;
    ti_c54x_config config;
    ti_c54x_cpu *cpu = NULL;

    (void)init_cpu;
    (void)emit_trace;

    fixture->program_words[0] = (uint16_t)(UINT16_C(0x4EE8) |
                                           ((uint16_t)(source & 1U) << 8U) |
                                           (uint16_t)(arf & 7U));
    fixture->program_words[1] = extension;

    endpoint = make_endpoint(&fixture->common);
    bus = make_bus(&fixture->common, &endpoint);
    endpoint.context = fixture;
    endpoint.advance = dst_advance_data;
    endpoint.cancel = dst_cancel_data;
    bus.context = fixture;
    bus.program_read_word = dst_read_program;
    bus.data_endpoint = endpoint;
    trace = (ti_c54x_trace_sink){TI_C54X_TRACE_MASK_ALL, fixture,
                                 dst_emit_trace};
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
                           &cpu->accumulators,
                           source == 0U ? TI_C54X_ACCUMULATOR_A
                                        : TI_C54X_ACCUMULATOR_B,
                           source_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           source == 0U ? TI_C54X_ACCUMULATOR_B
                                        : TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x0000112233)) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void dst_set_status(ti_c54x_cpu *cpu, uint8_t sxm, uint8_t sst,
                           int sxm_known, int sst_known)
{
    TI_C54X_TEST_CHECK(sxm <= 1U);
    TI_C54X_TEST_CHECK(sst <= 1U);
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 &
                   (uint16_t)~TI_C54X_ST1_SXM_MASK);
    if (sxm != 0U) {
        cpu->arch_state.status.st1 =
            (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_SXM_MASK);
    }
    cpu->arch_state.status.pmst =
        (uint16_t)(cpu->arch_state.status.pmst &
                   (uint16_t)~TI_C54X_PMST_SST_MASK);
    if (sst != 0U) {
        cpu->arch_state.status.pmst =
            (uint16_t)(cpu->arch_state.status.pmst | TI_C54X_PMST_SST_MASK);
    }
    if (sxm_known != 0) {
        cpu->arch_state.status.st1_known_mask =
            (uint16_t)(cpu->arch_state.status.st1_known_mask |
                       TI_C54X_ST1_SXM_MASK);
    } else {
        cpu->arch_state.status.st1_known_mask =
            (uint16_t)(cpu->arch_state.status.st1_known_mask &
                       (uint16_t)~TI_C54X_ST1_SXM_MASK);
    }
    if (sst_known != 0) {
        cpu->arch_state.status.pmst_known_mask =
            (uint16_t)(cpu->arch_state.status.pmst_known_mask |
                       TI_C54X_PMST_SST_MASK);
    } else {
        cpu->arch_state.status.pmst_known_mask =
            (uint16_t)(cpu->arch_state.status.pmst_known_mask &
                       (uint16_t)~TI_C54X_PMST_SST_MASK);
    }
}

static uint32_t dst_expected(uint64_t raw, uint8_t sxm, uint8_t sst)
{
    if (sst == 0U) {
        return (uint32_t)raw;
    }
    if (sxm == 0U) {
        return raw > UINT64_C(0xFFFFFFFF) ? UINT32_MAX : (uint32_t)raw;
    }
    {
        const int64_t signed_raw = ti_c54x_u40_sign_extend(raw);
        if (signed_raw > INT64_C(0x7FFFFFFF)) {
            return UINT32_C(0x7FFFFFFF);
        }
        if (signed_raw < -INT64_C(0x80000000)) {
            return UINT32_C(0x80000000);
        }
    }
    return (uint32_t)raw;
}

static void dst_check_ready(dst_fixture *fixture, ti_c54x_cpu *cpu,
                            uint8_t source, uint8_t arf, uint16_t ar_before,
                            uint16_t extension, uint64_t source_value,
                            uint8_t sxm, uint8_t sst)
{
    const uint16_t first = (uint16_t)(ar_before + extension);
    const uint16_t second = (first & 1U) == 0U ? (uint16_t)(first + 1U)
                                               : (uint16_t)(first - 1U);
    const uint32_t expected = dst_expected(source_value, sxm, sst);
    const ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

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
    TI_C54X_TEST_CHECK(fixture->requests[0].write_value_word ==
                       (uint16_t)(expected >> 16U));
    TI_C54X_TEST_CHECK(fixture->requests[1].write_value_word ==
                       (uint16_t)expected);
    TI_C54X_TEST_CHECK(fixture->ids[0] != UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture->ids[1] == fixture->ids[0] + UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == first);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(fixture->visible_writes[0] ==
                       (uint16_t)(expected >> 16U));
    TI_C54X_TEST_CHECK(fixture->visible_writes[1] == (uint16_t)expected);
    TI_C54X_TEST_CHECK(source == 0U ? cpu->accumulators.a_raw40 == source_value
                                    : cpu->accumulators.b_raw40 == source_value);
}

static void test_ready_saturation_and_source_copy(void)
{
    static const uint64_t values[] = {
        UINT64_C(0x0000000000), UINT64_C(0x0000FFFFFFFF),
        UINT64_C(0x0001000000), UINT64_C(0x007FFFFFFF),
        UINT64_C(0x0080000000), UINT64_C(0xFF7FFFFFFF),
        UINT64_C(0xFF80000000), UINT64_C(0xFF80000001),
        UINT64_C(0xFFFFFFFFFF)};

    for (uint8_t source = 0U; source <= 1U; ++source) {
        for (uint8_t sxm = 0U; sxm <= 1U; ++sxm) {
            for (uint8_t sst = 0U; sst <= 1U; ++sst) {
                for (size_t index = 0U;
                     index < sizeof(values) / sizeof(values[0]); ++index) {
                    dst_fixture fixture = {.mode = DST_READY,
                                            .fault_status = TI_C54X_BUS_FAULT};
                    _Alignas(max_align_t) unsigned char storage[1024] = {0};
                    ti_c54x_error status;
                    const uint16_t ar_before =
                        sst == 0U ? UINT16_C(0x3000) : UINT16_C(0x3001);
                    ti_c54x_cpu *cpu = dst_init(
                        &fixture, storage, source, UINT8_C(3), UINT16_C(2),
                        ar_before, values[index], &status);

                    dst_set_status(cpu, sxm, sst, 1, 1);
                    dst_check_ready(&fixture, cpu, source, UINT8_C(3),
                                    ar_before, UINT16_C(2), values[index],
                                    sxm, sst);
                    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                                       TI_C54X_ERROR_OK);
                }
            }
        }
    }
}

static void test_unknown_inputs_reject_before_endpoint(void)
{
    for (uint8_t unknown = 0U; unknown < 3U; ++unknown) {
        dst_fixture fixture = {.mode = DST_READY,
                                .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = dst_init(&fixture, storage, 0U, 4U, 2U, 0x3100U,
                                    UINT64_C(0xFF80000000), &status);
        dst_set_status(cpu, 1U, 1U, unknown != 1U, unknown != 2U);
        if (unknown == 0U) {
            cpu->accumulators.known_mask = (uint8_t)(
                cpu->accumulators.known_mask & (uint8_t)~UINT8_C(1));
        }
        {
            const ti_c54x_arch_state before = cpu->arch_state;
            const ti_c54x_accumulator_state before_accumulators =
                cpu->accumulators;
            const ti_c54x_run_result result =
                ti_c54x_cpu_run(cpu, UINT64_C(1));
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
            TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
            TI_C54X_TEST_CHECK(fixture.program_reads == 2U);
            TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before,
                                      sizeof(before)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                      sizeof(before_accumulators)) == 0);
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void test_wait_first_and_second_resume_without_refetch(void)
{
    for (dst_mode mode = DST_WAIT_FIRST; mode <= DST_WAIT_SECOND; ++mode) {
        dst_fixture fixture = {.mode = mode,
                                .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = dst_init(&fixture, storage, 1U, 5U, 2U, 0x3200U,
                                    UINT64_C(0x123456789A), &status);
        dst_set_status(cpu, 1U, 1U, 1, 1);
        {
            const ti_c54x_run_result wait = ti_c54x_cpu_run(cpu, UINT64_C(1));
            TI_C54X_TEST_CHECK(wait.reason == TI_C54X_STOP_BUS_WAIT);
            TI_C54X_TEST_CHECK(wait.instructions_attempted == UINT64_C(1));
            TI_C54X_TEST_CHECK(wait.instructions_retired == UINT64_C(0));
            TI_C54X_TEST_CHECK(fixture.program_reads == 2U);
            TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
            TI_C54X_TEST_CHECK(cpu->arch_state.ar[5] == UINT16_C(0x3200));
        }
        fixture.mode = DST_READY;
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
            if (mode == DST_WAIT_FIRST) {
                TI_C54X_TEST_CHECK(fixture.ids[1] == fixture.ids[0]);
                TI_C54X_TEST_CHECK(fixture.ids[2] ==
                                   fixture.ids[0] + UINT64_C(1));
            } else {
                TI_C54X_TEST_CHECK(fixture.ids[1] ==
                                   fixture.ids[0] + UINT64_C(1));
                TI_C54X_TEST_CHECK(fixture.ids[2] == fixture.ids[1]);
            }
            TI_C54X_TEST_CHECK(fixture.requests[0].address_word == 0x3202U);
            TI_C54X_TEST_CHECK(fixture.requests[1].address_word ==
                               (mode == DST_WAIT_FIRST ? 0x3202U : 0x3203U));
            TI_C54X_TEST_CHECK(fixture.requests[2].address_word == 0x3203U);
            TI_C54X_TEST_CHECK(cpu->arch_state.ar[5] == UINT16_C(0x3202));
            TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(2));
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void test_fault_first_and_second_keep_architecture_transactional(void)
{
    for (dst_mode mode = DST_FAULT_FIRST; mode <= DST_FAULT_SECOND; ++mode) {
        dst_fixture fixture = {.mode = mode,
                                .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = dst_init(&fixture, storage, 0U, 6U, 2U, 0x3300U,
                                    UINT64_C(0x000100000000), &status);
        dst_set_status(cpu, 0U, 1U, 1, 1);
        {
            const uint64_t before_a = cpu->accumulators.a_raw40;
            const ti_c54x_run_result result =
                ti_c54x_cpu_run(cpu, UINT64_C(1));
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
            TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
            TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
            TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
            TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
            TI_C54X_TEST_CHECK(cpu->arch_state.ar[6] == UINT16_C(0x3300));
            TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 == before_a);
            TI_C54X_TEST_CHECK(fixture.program_reads == 2U);
            TI_C54X_TEST_CHECK(fixture.ready_calls ==
                               (mode == DST_FAULT_SECOND ? 1U : 0U));
            TI_C54X_TEST_CHECK(fixture.endpoint_calls ==
                               (mode == DST_FAULT_SECOND ? 2U : 1U));
            if (mode == DST_FAULT_SECOND) {
                TI_C54X_TEST_CHECK(fixture.visible_writes[0] == UINT16_C(0xFFFF));
                TI_C54X_TEST_CHECK(fixture.visible_writes[1] == UINT16_C(0));
                TI_C54X_TEST_CHECK(fixture.ids[1] ==
                                   fixture.ids[0] + UINT64_C(1));
            } else {
                TI_C54X_TEST_CHECK(fixture.visible_writes[0] == UINT16_C(0));
            }
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

int main(void)
{
    test_ready_saturation_and_source_copy();
    test_unknown_inputs_reject_before_endpoint();
    test_wait_first_and_second_resume_without_refetch();
    test_fault_first_and_second_keep_architecture_transactional();
    return 0;
}
