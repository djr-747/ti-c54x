/* Module contract
 * Owns: public-CPU RED proofs for immediate-READY and resumable fixed-MOD2
 * `LD *ARx+,dst`, including SXM result selection and atomic rejection paths.
 * Does not own: catalog/decode or pure load effects, address-planner internals,
 * endpoint reducer behavior, production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/foundation/arithmetic40.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint16_t
ld_word(uint8_t destination, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x1090) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_cpu *
init_load(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
          bool with_endpoint, uint8_t arf, uint16_t ar_value, uint8_t sxm,
          ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, with_endpoint,
                                            status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, arf, ar_value);
    ti_c54x_cpu_isa_seed_sxm(cpu, sxm);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x1111222233)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000aabbcc)) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void
check_read_request(const ti_c54x_cpu_isa_memory_fixture *fixture,
                   uint16_t old_ar)
{
    TI_C54X_TEST_CHECK(fixture->requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word == old_ar);
    TI_C54X_TEST_CHECK(fixture->requests[0].write_value_word == UINT16_C(0));
}

static void
test_ready_forms_sign_extend_and_postincrement(void)
{
    for (uint8_t destination = 0U; destination <= UINT8_C(1); ++destination) {
        for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
            const uint8_t arf = destination == UINT8_C(0) ? UINT8_C(2)
                                                           : UINT8_C(5);
            const uint16_t old_ar = UINT16_C(0x2110);
            ti_c54x_cpu_isa_memory_fixture fixture = {
                .mode = TI_C54X_CPU_ISA_MEMORY_READY,
                .program_word = ld_word(destination, arf),
                .read_value_word = UINT16_C(0x8001),
                .fault_status = TI_C54X_BUS_FAULT};
            _Alignas(max_align_t) unsigned char storage[1024] = {0};
            ti_c54x_error status;
            ti_c54x_cpu *cpu = init_load(&fixture, storage, true, arf, old_ar,
                                         sxm, &status);
            ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));
            ti_c54x_state state;
            const uint64_t expected =
                sxm == UINT8_C(0)
                    ? UINT64_C(0x8001)
                    : (TI_C54X_U40_MASK & ~UINT64_C(0xffff)) |
                          UINT64_C(0x8001);

            (void)status;
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
            TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
            TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
            TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
            TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
            TI_C54X_TEST_CHECK(result.opcode_word == fixture.program_word);
            TI_C54X_TEST_CHECK(result.opcode_valid);
            TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
            TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
            TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
            check_read_request(&fixture, old_ar);
            TI_C54X_TEST_CHECK(fixture.requests[0].address_word == old_ar);
            TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + UINT16_C(1));
            TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                               TI_C54X_ERROR_OK);
            TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(1));
            TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                               (destination == UINT8_C(0) ? expected
                                                           : UINT64_C(0x1111222233)));
            TI_C54X_TEST_CHECK(state.accumulator_b_raw40 ==
                               (destination == UINT8_C(1) ? expected
                                                           : UINT64_C(0x0000aabbcc)));
            TI_C54X_TEST_CHECK(state.instructions_attempted_total ==
                               UINT64_C(1));
            TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
            TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
        }
    }
}

static void
test_wait_resumes_before_fetch_without_false_stop(void)
{
    const uint8_t arf = UINT8_C(3);
    const uint16_t old_ar = UINT16_C(0x3100);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = ld_word(UINT8_C(1), arf),
        .read_value_word = UINT16_C(0x1234),
        .fault_status = TI_C54X_BUS_FAULT};
    const ti_c54x_trace_sink trace = {
        TI_C54X_TRACE_MASK_ALL, &fixture, ti_c54x_cpu_isa_emit_trace};
    const ti_c54x_config config = {UINT32_C(0), trace};
    const ti_c54x_bus bus = ti_c54x_cpu_isa_bus(&fixture, true);
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;
    ti_c54x_error status;

    status = ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config, &bus, &cpu);
    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, arf, old_ar);
    ti_c54x_cpu_isa_seed_sxm(cpu, UINT8_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
#if defined(TI_C54X_ENABLE_OBSERVERS) && TI_C54X_ENABLE_OBSERVERS
    TI_C54X_TEST_CHECK(fixture.fetch_events == 1U);
#endif
    TI_C54X_TEST_CHECK(fixture.stop_events == 0U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[1] == fixture.ids[0]);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + UINT16_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fault_does_not_commit_or_publish(void)
{
    const uint8_t arf = UINT8_C(1);
    const uint16_t old_ar = UINT16_C(0x4000);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_FAULT,
        .program_word = ld_word(UINT8_C(0), arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_load(&fixture, storage, true, arf, old_ar,
                                 UINT8_C(1), &status);
    const uint64_t before_a = cpu->accumulators.a_raw40;
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 == before_a);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_no_callback_and_boundary_reject_before_issue(void)
{
    {
        const uint8_t arf = UINT8_C(0);
        const uint16_t old_ar = UINT16_C(0x4200);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ld_word(UINT8_C(0), arf)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_load(&fixture, storage, false, arf, old_ar,
                                     UINT8_C(0), &status);
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
            .program_word = ld_word(UINT8_C(1), arf)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_load(&fixture, storage, true, arf,
                                     UINT16_MAX, UINT8_C(0), &status);
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
test_wait_cancellation_and_zero_budget_are_quiet(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_ALWAYS,
        .program_word = ld_word(UINT8_C(0), UINT8_C(2))};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_load(&fixture, storage, true, UINT8_C(2),
                                 UINT16_C(0x5000), UINT8_C(0), &status);
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
    test_ready_forms_sign_extend_and_postincrement();
    test_wait_resumes_before_fetch_without_false_stop();
    test_fault_does_not_commit_or_publish();
    test_no_callback_and_boundary_reject_before_issue();
    test_wait_cancellation_and_zero_budget_are_quiet();
    return 0;
}
