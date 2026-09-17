/* Module contract
 * Owns: public-CPU RED proofs for immediate-READY and resumable fixed-MOD2
 * `STH src,*ARx+`, including high-word publication and atomic rejection paths.
 * Does not own: catalog/decode or pure store effects, address-planner
 * internals, endpoint reducer behavior, production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include <stddef.h>
#include <stdint.h>

static uint16_t
sth_word(uint8_t source, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x8290) |
                      ((uint16_t)(source & UINT8_C(1)) << 8U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_cpu *
init_store(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
           bool with_endpoint, uint8_t arf, uint16_t ar_value,
           ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, with_endpoint,
                                            status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, arf, ar_value);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x123456789a)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x00abcdef01)) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void
check_write_request(const ti_c54x_cpu_isa_memory_fixture *fixture,
                    uint16_t old_ar, uint16_t expected_value)
{
    TI_C54X_TEST_CHECK(fixture->requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word == old_ar);
    TI_C54X_TEST_CHECK(fixture->requests[0].write_value_word ==
                       expected_value);
}

static void
test_ready_forms_publish_high_word_and_postincrement(void)
{
    for (uint8_t source = 0U; source <= UINT8_C(1); ++source) {
        const uint8_t arf = source == UINT8_C(0) ? UINT8_C(2) : UINT8_C(5);
        const uint16_t old_ar = UINT16_C(0x2110);
        const uint16_t expected = source == UINT8_C(0) ? UINT16_C(0x3456)
                                                       : UINT16_C(0xabcd);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = sth_word(source, arf),
            .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_store(&fixture, storage, true, arf, old_ar,
                                      &status);
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
        TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
        TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
        check_write_request(&fixture, old_ar, expected);
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + UINT16_C(1));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_wait_resumes_same_id_and_publishes_once(void)
{
    const uint8_t source = UINT8_C(1);
    const uint8_t arf = UINT8_C(4);
    const uint16_t old_ar = UINT16_C(0x3200);
    const uint16_t expected = UINT16_C(0xabcd);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = sth_word(source, arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_store(&fixture, storage, true, arf, old_ar,
                                  &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[1] == fixture.ids[0]);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
    check_write_request(&fixture, old_ar, expected);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(fixture.requests[1].write_value_word == expected);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + UINT16_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fault_no_callback_and_boundary_do_not_publish(void)
{
    {
        const uint8_t arf = UINT8_C(1);
        const uint16_t old_ar = UINT16_C(0x4000);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_FAULT,
            .program_word = sth_word(UINT8_C(0), arf),
            .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_store(&fixture, storage, true, arf, old_ar,
                                      &status);
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
    {
        const uint8_t arf = UINT8_C(0);
        const uint16_t old_ar = UINT16_C(0x4100);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = sth_word(UINT8_C(1), arf)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_store(&fixture, storage, false, arf, old_ar,
                                      &status);
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_NO_CALLBACK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
    {
        const uint8_t arf = UINT8_C(7);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = sth_word(UINT8_C(0), arf)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_store(&fixture, storage, true, arf,
                                      UINT16_MAX, &status);
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == UINT16_MAX);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_zero_budget_and_wait_cancellation(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_ALWAYS,
        .program_word = sth_word(UINT8_C(0), UINT8_C(2))};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_store(&fixture, storage, true, UINT8_C(2),
                                  UINT16_C(0x5000), &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(0));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_reads == 0U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(fixture.cancel_calls == 1U);
    TI_C54X_TEST_CHECK(cpu->instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_ready_forms_publish_high_word_and_postincrement();
    test_wait_resumes_same_id_and_publishes_once();
    test_fault_no_callback_and_boundary_do_not_publish();
    test_zero_budget_and_wait_cancellation();
    return 0;
}
