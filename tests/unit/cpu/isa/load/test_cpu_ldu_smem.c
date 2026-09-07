/* Module contract
 * Owns: public-CPU RED proofs for the evidence-backed fixed-MOD2
 * `LDU *ARx+,dst` load, including zero-extension, SXM independence,
 * resumable endpoint completion, and transactional rejection paths.
 * Does not own: catalog/decode or pure LDU effects, address-planner internals,
 * endpoint reducer behavior, production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint16_t
ldu_word(uint8_t destination, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x1290) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_cpu *
init_ldu(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
         bool with_endpoint, uint8_t arf, uint16_t ar_value,
         ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, with_endpoint,
                                            status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, arf, ar_value);
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
check_rejection_preserved(const ti_c54x_cpu *cpu,
                          const ti_c54x_arch_state *before,
                          const ti_c54x_accumulator_state *before_accumulators)
{
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == before->pc_word);
    TI_C54X_TEST_CHECK(cpu->arch_state.ifr == before->ifr);
    TI_C54X_TEST_CHECK(cpu->arch_state.rc == before->rc);
    TI_C54X_TEST_CHECK(cpu->arch_state.bk == before->bk);
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == before->sp);
    TI_C54X_TEST_CHECK(cpu->arch_state.scalar_known_mask ==
                       before->scalar_known_mask);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar_known_mask == before->ar_known_mask);
    for (uint8_t index = 0U; index < UINT8_C(8); ++index) {
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[index] == before->ar[index]);
    }
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == before->status.st0);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == before->status.st1);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst == before->status.pmst);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0_known_mask ==
                       before->status.st0_known_mask);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1_known_mask ==
                       before->status.st1_known_mask);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst_known_mask ==
                       before->status.pmst_known_mask);
    TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 == before_accumulators->a_raw40);
    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 == before_accumulators->b_raw40);
    TI_C54X_TEST_CHECK(cpu->accumulators.known_mask ==
                       before_accumulators->known_mask);
}

static void
test_ready_forms_zero_extend_and_postincrement(void)
{
    static const uint16_t values[] = {
        UINT16_C(0x0000), UINT16_C(0x8000), UINT16_C(0xffff)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1); ++destination) {
        for (size_t value_index = 0U;
             value_index < sizeof(values) / sizeof(values[0]); ++value_index) {
            const uint8_t arf = destination == UINT8_C(0) ? UINT8_C(2)
                                                           : UINT8_C(5);
            const uint16_t old_ar = UINT16_C(0x2110);
            ti_c54x_cpu_isa_memory_fixture fixture = {
                .mode = TI_C54X_CPU_ISA_MEMORY_READY,
                .program_word = ldu_word(destination, arf),
                .read_value_word = values[value_index],
                .fault_status = TI_C54X_BUS_FAULT};
            _Alignas(max_align_t) unsigned char storage[1024] = {0};
            ti_c54x_error status;
            ti_c54x_cpu *cpu = init_ldu(&fixture, storage, true, arf, old_ar,
                                        &status);
            const uint16_t before_st0 = cpu->arch_state.status.st0;
            const uint16_t before_st1 = cpu->arch_state.status.st1;
            const uint16_t before_pmst = cpu->arch_state.status.pmst;
            const uint16_t before_st0_known =
                cpu->arch_state.status.st0_known_mask;
            const uint16_t before_st1_known =
                cpu->arch_state.status.st1_known_mask;
            const uint16_t before_pmst_known =
                cpu->arch_state.status.pmst_known_mask;
            ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

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
            TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + 1U);
            TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                               (destination == UINT8_C(0)
                                    ? (uint64_t)values[value_index]
                                    : UINT64_C(0x1111222233)));
            TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                               (destination == UINT8_C(1)
                                    ? (uint64_t)values[value_index]
                                    : UINT64_C(0x0000aabbcc)));
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == before_st0);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == before_st1);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst == before_pmst);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st0_known_mask ==
                               before_st0_known);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st1_known_mask ==
                               before_st1_known);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst_known_mask ==
                               before_pmst_known);
            TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
        }
    }
}

static void
test_sxm_is_ignored_even_when_unknown(void)
{
    for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
        const uint8_t arf = UINT8_C(3);
        const uint16_t old_ar = UINT16_C(0x3100);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ldu_word(UINT8_C(1), arf),
            .read_value_word = UINT16_C(0x8001),
            .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_ldu(&fixture, storage, true, arf, old_ar,
                                    &status);
        ti_c54x_cpu_isa_seed_sxm(cpu, sxm);
        cpu->arch_state.status.st1_known_mask =
            (uint16_t)(cpu->arch_state.status.st1_known_mask &
                       (uint16_t)~TI_C54X_ST1_SXM_MASK);
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 == UINT64_C(0x8001));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + 1U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_wait_resumes_once_before_fetch(void)
{
    const uint8_t arf = UINT8_C(3);
    const uint16_t old_ar = UINT16_C(0x3200);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = ldu_word(UINT8_C(0), arf),
        .read_value_word = UINT16_C(0x8000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_ldu(&fixture, storage, true, arf, old_ar,
                                &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[1] == fixture.ids[0]);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + 1U);
    TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 == UINT64_C(0x8000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fault_rolls_back_without_publish(void)
{
    const uint8_t arf = UINT8_C(1);
    const uint16_t old_ar = UINT16_C(0x4000);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_FAULT,
        .program_word = ldu_word(UINT8_C(0), arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_ldu(&fixture, storage, true, arf, old_ar,
                                &status);
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
test_unknown_dependencies_and_boundary_reject_before_issue(void)
{
    const uint8_t arf = UINT8_C(4);
    const uint16_t old_ar = UINT16_C(0x4200);

    {
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ldu_word(UINT8_C(0), arf)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_ldu(&fixture, storage, true, arf, old_ar,
                                    &status);
        cpu->arch_state.ar_known_mask =
            (uint8_t)(cpu->arch_state.ar_known_mask &
                      (uint8_t)~(UINT8_C(1) << arf));
        const ti_c54x_arch_state before = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == before.ar[arf]);
        check_rejection_preserved(cpu, &before, &before_accumulators);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }

    {
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ldu_word(UINT8_C(0), arf)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_ldu(&fixture, storage, true, arf, old_ar,
                                    &status);
        cpu->arch_state.status.st0_known_mask =
            (uint16_t)(cpu->arch_state.status.st0_known_mask &
                       (uint16_t)~TI_C54X_ST0_ARP_MASK);
        const ti_c54x_arch_state before = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
        check_rejection_preserved(cpu, &before, &before_accumulators);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }

    {
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ldu_word(UINT8_C(0), arf)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_ldu(&fixture, storage, true, arf, old_ar,
                                    &status);
        cpu->arch_state.status.st1_known_mask =
            (uint16_t)(cpu->arch_state.status.st1_known_mask &
                       (uint16_t)~TI_C54X_ST1_CMPT_MASK);
        const ti_c54x_arch_state before = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
        check_rejection_preserved(cpu, &before, &before_accumulators);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }

    {
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ldu_word(UINT8_C(0), arf)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_ldu(&fixture, storage, true, arf, UINT16_MAX,
                                    &status);
        const ti_c54x_arch_state before = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == UINT16_MAX);
        check_rejection_preserved(cpu, &before, &before_accumulators);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

int
main(void)
{
    test_ready_forms_zero_extend_and_postincrement();
    test_sxm_is_ignored_even_when_unknown();
    test_wait_resumes_once_before_fetch();
    test_fault_rolls_back_without_publish();
    test_unknown_dependencies_and_boundary_reject_before_issue();
    return 0;
}
