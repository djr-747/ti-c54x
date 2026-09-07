/* Module contract
 * Owns: public-CPU RED proofs for the fixed-MOD2 `ST T,*ARx+` WORD16 store,
 * including exact T publication, resumable completion, and atomic rejection.
 * Does not own: production CPU behavior, catalog/decode or pure store
 * effects, address planning, shared fixtures, timing, diagnostics, or tests
 * outside this store leaf.
 */

#include "../fixture.h"

#include "../../../../../src/state/temporary/t_register.h"

#include <stddef.h>
#include <stdint.h>

static uint16_t
st_t_word(uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x8c90) | (uint16_t)(arf & UINT8_C(7)));
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
    cpu->arch_state.status.st0 = UINT16_C(0xa5c3);
    cpu->arch_state.status.st1 = UINT16_C(0x5a96);
    cpu->arch_state.status.pmst = UINT16_C(0xc35a);
    cpu->arch_state.status.st0_known_mask = UINT16_MAX;
    cpu->arch_state.status.st1_known_mask = UINT16_MAX;
    cpu->arch_state.status.pmst_known_mask = UINT16_MAX;
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    return cpu;
}

static void
seed_t(ti_c54x_cpu *cpu, uint16_t value)
{
    TI_C54X_TEST_CHECK(ti_c54x_t_register_write(&cpu->t_register, value) ==
                       TI_C54X_T_REGISTER_OK);
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
check_arch_unchanged(const ti_c54x_cpu *cpu, const ti_c54x_arch_state *before)
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
}

static void
test_ready_publishes_exact_t_for_all_arfs(void)
{
    static const uint16_t values[] = {
        UINT16_C(0x0000), UINT16_C(0x7fff),
        UINT16_C(0x8000), UINT16_C(0xffff)};

    for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
        for (size_t value_index = 0U;
             value_index < sizeof(values) / sizeof(values[0]);
             ++value_index) {
            const uint16_t old_ar =
                (uint16_t)(UINT16_C(0x2100) +
                           (uint16_t)(arf * UINT16_C(4)));
            ti_c54x_cpu_isa_memory_fixture fixture = {
                .mode = TI_C54X_CPU_ISA_MEMORY_READY,
                .program_word = st_t_word(arf),
                .fault_status = TI_C54X_BUS_FAULT};
            _Alignas(max_align_t) unsigned char storage[1024] = {0};
            ti_c54x_error status;
            ti_c54x_cpu *cpu = init_store(&fixture, storage, true, arf, old_ar,
                                          &status);
            const ti_c54x_accumulator_state before_accumulators =
                cpu->accumulators;
            seed_t(cpu, values[value_index]);
            const ti_c54x_t_register_state before_t = cpu->t_register;
            const ti_c54x_status_register_state before_status =
                cpu->arch_state.status;
            ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));
            ti_c54x_state state;

            (void)status;
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
            TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
            TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
            TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
            TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
            TI_C54X_TEST_CHECK(result.opcode_word == fixture.program_word);
            TI_C54X_TEST_CHECK(result.opcode_valid);
            TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
            TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
            TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
            TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
            check_write_request(&fixture, old_ar, values[value_index]);
            TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] ==
                               (uint16_t)(old_ar + UINT16_C(1)));
            TI_C54X_TEST_CHECK(cpu->t_register.raw_value == before_t.raw_value);
            TI_C54X_TEST_CHECK(cpu->t_register.known == before_t.known);
            TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                               before_accumulators.a_raw40);
            TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                               before_accumulators.b_raw40);
            TI_C54X_TEST_CHECK(cpu->accumulators.known_mask ==
                               before_accumulators.known_mask);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == before_status.st0);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == before_status.st1);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst == before_status.pmst);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st0_known_mask ==
                               before_status.st0_known_mask);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st1_known_mask ==
                               before_status.st1_known_mask);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst_known_mask ==
                               before_status.pmst_known_mask);
            TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                               TI_C54X_ERROR_OK);
            TI_C54X_TEST_CHECK(state.instructions_attempted_total ==
                               UINT64_C(1));
            TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
            TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
        }
    }
}

static void
test_wait_resumes_same_transaction_without_refetch(void)
{
    const uint8_t arf = UINT8_C(3);
    const uint16_t old_ar = UINT16_C(0x3200);
    const uint16_t value = UINT16_C(0x8000);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = st_t_word(arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_store(&fixture, storage, true, arf, old_ar,
                                  &status);
    seed_t(cpu, value);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == value);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[0] == fixture.ids[1]);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(fixture.requests[1].write_value_word == value);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + UINT16_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fault_and_missing_endpoint_roll_back(void)
{
    {
        const uint8_t arf = UINT8_C(1);
        const uint16_t old_ar = UINT16_C(0x4000);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_FAULT,
            .program_word = st_t_word(arf),
            .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_store(&fixture, storage, true, arf, old_ar,
                                      &status);
        seed_t(cpu, UINT16_C(0x2468));
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_t_register_state before_t = cpu->t_register;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
        check_arch_unchanged(cpu, &before_arch);
        TI_C54X_TEST_CHECK(cpu->t_register.raw_value == before_t.raw_value);
        TI_C54X_TEST_CHECK(cpu->t_register.known == before_t.known);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
    {
        const uint8_t arf = UINT8_C(2);
        const uint16_t old_ar = UINT16_C(0x4100);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = st_t_word(arf)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_store(&fixture, storage, false, arf, old_ar,
                                      &status);
        seed_t(cpu, UINT16_C(0x1357));
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_NO_CALLBACK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        check_arch_unchanged(cpu, &before_arch);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_unknown_dependencies_and_boundary_reject_before_issue(void)
{
    const uint8_t arf = UINT8_C(4);
    const uint16_t old_ar = UINT16_C(0x4200);

    for (unsigned dependency = 0U; dependency < 4U; ++dependency) {
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = st_t_word(arf)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_store(&fixture, storage, true, arf,
                                      dependency == 3U ? UINT16_MAX : old_ar,
                                      &status);
        seed_t(cpu, UINT16_C(0x2468));
        if (dependency == 0U) {
            cpu->arch_state.ar_known_mask = (uint8_t)(
                cpu->arch_state.ar_known_mask &
                (uint8_t)~(UINT8_C(1) << arf));
        } else if (dependency == 1U) {
            cpu->arch_state.status.st0_known_mask = (uint16_t)(
                cpu->arch_state.status.st0_known_mask &
                (uint16_t)~TI_C54X_ST0_ARP_MASK);
        } else if (dependency == 2U) {
            cpu->arch_state.status.st1_known_mask = (uint16_t)(
                cpu->arch_state.status.st1_known_mask &
                (uint16_t)~TI_C54X_ST1_CMPT_MASK);
        }
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_t_register_state before_t = cpu->t_register;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        check_arch_unchanged(cpu, &before_arch);
        TI_C54X_TEST_CHECK(cpu->t_register.raw_value == before_t.raw_value);
        TI_C54X_TEST_CHECK(cpu->t_register.known == before_t.known);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_reset_unknown_t_rejects_before_data_callback(void)
{
    const uint8_t arf = UINT8_C(5);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = st_t_word(arf)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_store(&fixture, storage, true, arf,
                                  UINT16_C(0x4300), &status);
    seed_t(cpu, UINT16_C(0xcafe));
    TI_C54X_TEST_CHECK(ti_c54x_t_register_reset(&cpu->t_register) ==
                       TI_C54X_T_REGISTER_OK);
    const ti_c54x_arch_state before_arch = cpu->arch_state;
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    check_arch_unchanged(cpu, &before_arch);
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(!cpu->t_register.known);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_ready_publishes_exact_t_for_all_arfs();
    test_wait_resumes_same_transaction_without_refetch();
    test_fault_and_missing_endpoint_roll_back();
    test_unknown_dependencies_and_boundary_reject_before_issue();
    test_reset_unknown_t_rejects_before_data_callback();
    return 0;
}
