/* Module contract
 * Owns: public-CPU RED proofs for the fixed-MOD2 `LD Smem,16,dst` load,
 * including SXM-controlled extension, logical left shift, old-AR reads,
 * postincrement, resumable completion, and transactional rejection.
 * Does not own: production CPU behavior, catalog/decode or pure load effects,
 * address planning, shared fixture implementation, timing, diagnostics, or
 * test registration outside the load leaf.
 */

#include "../fixture.h"

#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../../src/state/register_layout.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
ld_shift16_word(uint8_t destination, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x4490) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static uint64_t
expected_shift16(uint16_t read_value, uint8_t sxm)
{
    uint64_t extended = (uint64_t)read_value;

    if (sxm != UINT8_C(0) && (read_value & UINT16_C(0x8000)) != 0U) {
        extended = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) |
                   (uint64_t)read_value;
    }
    return (extended << UINT8_C(16)) & TI_C54X_U40_MASK;
}

static ti_c54x_cpu *
init_load(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
          bool with_endpoint, uint8_t destination, uint8_t arf,
          uint16_t ar_value, uint8_t sxm, ti_c54x_error *status)
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
                           UINT64_C(0x0000AABBCC)) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;

    /* Make C, TC, and both accumulator overflow bits visibly non-default. */
    cpu->arch_state.status.st0 = (uint16_t)(
        cpu->arch_state.status.st0 | TI_C54X_ST0_TC_MASK |
        TI_C54X_ST0_C_MASK | TI_C54X_ST0_OVA_MASK | TI_C54X_ST0_OVB_MASK);
    (void)destination;
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
test_ready_forms_shift16_and_postincrement(void)
{
    static const uint16_t values[] = {
        UINT16_C(0x0000), UINT16_C(0x0001), UINT16_C(0x7FFF),
        UINT16_C(0x8000), UINT16_C(0xFFFF)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1); ++destination) {
        for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
            for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
                for (size_t value_index = 0U;
                     value_index < sizeof(values) / sizeof(values[0]);
                     ++value_index) {
                    const uint16_t old_ar =
                        (uint16_t)(UINT16_C(0x2100) +
                                   (uint16_t)(arf * UINT8_C(4)));
                    ti_c54x_cpu_isa_memory_fixture fixture = {
                        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
                        .program_word = ld_shift16_word(destination, arf),
                        .read_value_word = values[value_index],
                        .fault_status = TI_C54X_BUS_FAULT};
                    _Alignas(max_align_t) unsigned char storage[1024] = {0};
                    ti_c54x_error status;
                    ti_c54x_cpu *cpu = init_load(
                        &fixture, storage, true, destination, arf, old_ar,
                        sxm, &status);
                    const ti_c54x_accumulator_state before_accumulators =
                        cpu->accumulators;
                    const ti_c54x_status_register_state before_status =
                        cpu->arch_state.status;
                    const ti_c54x_t_register_state before_t = cpu->t_register;
                    ti_c54x_run_result result =
                        ti_c54x_cpu_run(cpu, UINT64_C(1));
                    ti_c54x_state state;

                    (void)status;
                    TI_C54X_TEST_CHECK(result.reason ==
                                       TI_C54X_STOP_BUDGET_EXHAUSTED);
                    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
                    TI_C54X_TEST_CHECK(result.instructions_attempted ==
                                       UINT64_C(1));
                    TI_C54X_TEST_CHECK(result.instructions_retired ==
                                       UINT64_C(1));
                    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
                    TI_C54X_TEST_CHECK(result.opcode_word ==
                                       fixture.program_word);
                    TI_C54X_TEST_CHECK(result.opcode_valid);
                    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
                    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
                    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
                    check_read_request(&fixture, old_ar);
                    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] ==
                                       (uint16_t)(old_ar + UINT16_C(1)));
                    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state.status,
                                              &before_status,
                                              sizeof(before_status)) == 0);
                    TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                              sizeof(before_t)) == 0);
                    if (destination == UINT8_C(0)) {
                        TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                                           expected_shift16(values[value_index],
                                                            sxm));
                        TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                                           before_accumulators.b_raw40);
                    } else {
                        TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                                           expected_shift16(values[value_index],
                                                            sxm));
                        TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                                           before_accumulators.a_raw40);
                    }
                    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                                       TI_C54X_ERROR_OK);
                    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(1));
                    TI_C54X_TEST_CHECK(state.instructions_attempted_total ==
                                       UINT64_C(1));
                    TI_C54X_TEST_CHECK(state.instructions_retired_total ==
                                       UINT64_C(1));
                    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                                       TI_C54X_ERROR_OK);
                }
            }
        }
    }
}

static void
test_wait_resumes_same_transaction_without_refetch(void)
{
    const uint8_t destination = UINT8_C(1);
    const uint8_t arf = UINT8_C(3);
    const uint16_t old_ar = UINT16_C(0x3200);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = ld_shift16_word(destination, arf),
        .read_value_word = UINT16_C(0x8000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_load(&fixture, storage, true, destination, arf,
                                 old_ar, UINT8_C(1), &status);
    const ti_c54x_arch_state before_arch = cpu->arch_state;
    const ti_c54x_accumulator_state before_accumulators = cpu->accumulators;
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                              sizeof(before_arch)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                              sizeof(before_accumulators)) == 0);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[0] == fixture.ids[1]);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + UINT16_C(1));
    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                       expected_shift16(fixture.read_value_word, UINT8_C(1)));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    (void)status;
}

static void
test_fault_and_missing_endpoint_roll_back(void)
{
    const uint8_t destination = UINT8_C(0);
    const uint8_t arf = UINT8_C(1);
    const uint16_t old_ar = UINT16_C(0x4000);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_FAULT,
        .program_word = ld_shift16_word(destination, arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_load(&fixture, storage, true, destination, arf,
                                 old_ar, UINT8_C(1), &status);
    const ti_c54x_arch_state before_arch = cpu->arch_state;
    const ti_c54x_accumulator_state before_accumulators = cpu->accumulators;
    const ti_c54x_t_register_state before_t = cpu->t_register;
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                              sizeof(before_arch)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                              sizeof(before_accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t, sizeof(before_t)) ==
                       0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);

    memset(storage, 0, sizeof(storage));
    fixture = (ti_c54x_cpu_isa_memory_fixture){
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = ld_shift16_word(destination, arf)};
    cpu = init_load(&fixture, storage, false, destination, arf, old_ar,
                    UINT8_C(0), &status);
    const ti_c54x_arch_state before_no_endpoint = cpu->arch_state;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_NO_CALLBACK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_no_endpoint,
                              sizeof(before_no_endpoint)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_dependencies_and_boundary_reject_before_issue(void)
{
    const uint8_t destination = UINT8_C(1);
    const uint8_t arf = UINT8_C(4);
    for (unsigned dependency = 0U; dependency < 5U; ++dependency) {
        const uint16_t old_ar =
            dependency == 4U ? UINT16_MAX : UINT16_C(0x4200);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ld_shift16_word(destination, arf),
            .read_value_word = UINT16_C(0xFFFF),
            .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_load(&fixture, storage, true, destination,
                                     arf, old_ar, UINT8_C(1), &status);

        if (dependency == 0U) {
            cpu->arch_state.status.st1_known_mask = (uint16_t)(
                cpu->arch_state.status.st1_known_mask &
                (uint16_t)~TI_C54X_ST1_SXM_MASK);
        } else if (dependency == 1U) {
            cpu->arch_state.status.st1_known_mask = (uint16_t)(
                cpu->arch_state.status.st1_known_mask &
                (uint16_t)~TI_C54X_ST1_CMPT_MASK);
        } else if (dependency == 2U) {
            cpu->arch_state.status.st0_known_mask = (uint16_t)(
                cpu->arch_state.status.st0_known_mask &
                (uint16_t)~TI_C54X_ST0_ARP_MASK);
        } else if (dependency == 3U) {
            cpu->arch_state.ar_known_mask = (uint8_t)(
                cpu->arch_state.ar_known_mask & (uint8_t)~(UINT8_C(1) << arf));
        }

        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators = cpu->accumulators;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
        (void)status;
    }
}

int
main(void)
{
    test_ready_forms_shift16_and_postincrement();
    test_wait_resumes_same_transaction_without_refetch();
    test_fault_and_missing_endpoint_roll_back();
    test_unknown_dependencies_and_boundary_reject_before_issue();
    return 0;
}
