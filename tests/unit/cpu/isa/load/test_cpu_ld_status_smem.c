/* Module contract
 * Owns: public-CPU RED proofs for fixed-MOD2 `LD Smem,DP` and
 * `LD Smem,ASM`, including status-field transfer, old-AR reads,
 * postincrement, resumable completion, and transactional rejection.
 * Does not own: production CPU behavior, catalog/decode or pure load effects,
 * address planning, shared fixture implementation, timing, diagnostics, or
 * test registration outside the load leaf.
 */

#include "../fixture.h"

#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../../src/state/temporary/t_register.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum ld_status_target {
    LD_STATUS_DP = 0,
    LD_STATUS_ASM = 1
} ld_status_target;

static uint16_t
ld_status_word(ld_status_target target, uint8_t arf)
{
    const uint16_t base = target == LD_STATUS_DP ? UINT16_C(0x4690)
                                                 : UINT16_C(0x3290);
    return (uint16_t)(base | (uint16_t)(arf & UINT8_C(7)));
}

static uint16_t
ld_status_expected_value(ld_status_target target, uint16_t read_value)
{
    return (uint16_t)(read_value &
                      (target == LD_STATUS_DP ? TI_C54X_ST0_DP_MASK
                                              : TI_C54X_ST1_ASM_MASK));
}

static ti_c54x_cpu *
init_cpu(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
         bool with_endpoint, ld_status_target target, uint8_t arf,
         uint16_t ar_value, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, with_endpoint,
                                            status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);

    /* Seed raw status values and knownness before typed dependencies.  The
     * typed CMPT/ARP writes below must preserve all surrounding bits while
     * making the fixed-MOD2 planner inputs known and zero. */
    cpu->arch_state.status.st0 = UINT16_C(0xA5C3);
    cpu->arch_state.status.st1 = UINT16_C(0x5A96);
    cpu->arch_state.status.pmst = UINT16_C(0xC35A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    ti_c54x_cpu_isa_seed_address(cpu, arf, ar_value);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x123456789A)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x00ABCDEF01)) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
    (void)target;
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
check_status_after_ready(const ti_c54x_cpu *cpu,
                         const ti_c54x_status_register_state *before,
                         ld_status_target target, uint16_t read_value)
{
    const uint16_t expected = ld_status_expected_value(target, read_value);

    if (target == LD_STATUS_DP) {
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 ==
                           (uint16_t)((before->st0 &
                                       (uint16_t)~TI_C54X_ST0_DP_MASK) |
                                      expected));
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == before->st1);
    } else {
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == before->st0);
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 ==
                           (uint16_t)((before->st1 &
                                       (uint16_t)~TI_C54X_ST1_ASM_MASK) |
                                      expected));
    }
    TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst == before->pmst);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0_known_mask ==
                       before->st0_known_mask);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1_known_mask ==
                       before->st1_known_mask);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst_known_mask ==
                       before->pmst_known_mask);
}

static void
test_ready_all_arfs_and_values(void)
{
    static const uint16_t values[] = {UINT16_C(0x0000), UINT16_C(0x01FF),
                                      UINT16_C(0x1FFF), UINT16_C(0xFFFF)};

    for (ld_status_target target = LD_STATUS_DP;
         target <= LD_STATUS_ASM; ++target) {
        for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
            for (size_t value_index = 0U;
                 value_index < sizeof(values) / sizeof(values[0]);
                 ++value_index) {
                const uint16_t old_ar =
                    (uint16_t)(UINT16_C(0x2100) + (uint16_t)(arf * 4U));
                ti_c54x_cpu_isa_memory_fixture fixture = {
                    .mode = TI_C54X_CPU_ISA_MEMORY_READY,
                    .program_word = ld_status_word(target, arf),
                    .read_value_word = values[value_index],
                    .fault_status = TI_C54X_BUS_FAULT};
                _Alignas(max_align_t) unsigned char storage[1024] = {0};
                ti_c54x_error status;
                ti_c54x_cpu *cpu = init_cpu(
                    &fixture, storage, true, target, arf, old_ar, &status);
                const ti_c54x_status_register_state before_status =
                    cpu->arch_state.status;
                const ti_c54x_accumulator_state before_accumulators =
                    cpu->accumulators;
                const ti_c54x_t_register_state before_t = cpu->t_register;
                ti_c54x_run_result result =
                    ti_c54x_cpu_run(cpu, UINT64_C(1));
                ti_c54x_state state;

                TI_C54X_TEST_CHECK(result.reason ==
                                   TI_C54X_STOP_BUDGET_EXHAUSTED);
                TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
                TI_C54X_TEST_CHECK(result.instructions_attempted ==
                                   UINT64_C(1));
                TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
                TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
                TI_C54X_TEST_CHECK(result.opcode_word == fixture.program_word);
                TI_C54X_TEST_CHECK(result.opcode_valid);
                TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
                TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
                TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
                check_read_request(&fixture, old_ar);
                TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + 1U);
                check_status_after_ready(&cpu[0], &before_status, target,
                                         values[value_index]);
                TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators,
                                          &before_accumulators,
                                          sizeof(before_accumulators)) == 0);
                TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                          sizeof(before_t)) == 0);
                TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                                   TI_C54X_ERROR_OK);
                TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(1));
                TI_C54X_TEST_CHECK(state.instructions_attempted_total ==
                                   UINT64_C(1));
                TI_C54X_TEST_CHECK(state.instructions_retired_total ==
                                   UINT64_C(1));
                if (target == LD_STATUS_DP) {
                    TI_C54X_TEST_CHECK(state.dp ==
                                       ld_status_expected_value(
                                           target, values[value_index]));
                    TI_C54X_TEST_CHECK(state.dp_known);
                } else {
                    TI_C54X_TEST_CHECK(state.asm_raw ==
                                       ld_status_expected_value(
                                           target, values[value_index]));
                    TI_C54X_TEST_CHECK(state.asm_known);
                }
                TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                                   TI_C54X_ERROR_OK);
                (void)status;
            }
        }
    }
}

static void
test_wait_resumes_same_transaction_without_refetch(void)
{
    const ld_status_target target = LD_STATUS_DP;
    const uint8_t arf = UINT8_C(3);
    const uint16_t old_ar = UINT16_C(0x3200);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = ld_status_word(target, arf),
        .read_value_word = UINT16_C(0x1FFF),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, true, target, arf, old_ar, &status);
    const ti_c54x_status_register_state before_status = cpu->arch_state.status;
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state.status, &before_status,
                              sizeof(before_status)) == 0);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[0] == fixture.ids[1]);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + 1U);
    check_status_after_ready(cpu, &before_status, target,
                             fixture.read_value_word);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    (void)status;
}

static void
test_fault_and_no_callback_roll_back(void)
{
    const ld_status_target target = LD_STATUS_ASM;
    const uint8_t arf = UINT8_C(1);
    const uint16_t old_ar = UINT16_C(0x4000);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_FAULT,
        .program_word = ld_status_word(target, arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, true, target, arf, old_ar, &status);
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
        .program_word = ld_status_word(target, arf)};
    cpu = init_cpu(&fixture, storage, false, target, arf, old_ar, &status);
    const ti_c54x_arch_state no_callback_before = cpu->arch_state;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_NO_CALLBACK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &no_callback_before,
                              sizeof(no_callback_before)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    (void)status;
}

static void
test_unknown_dependencies_and_ar_boundary_reject(void)
{
    for (unsigned dependency = 0U; dependency < 4U; ++dependency) {
        const ld_status_target target =
            dependency % 2U == 0U ? LD_STATUS_DP : LD_STATUS_ASM;
        const uint8_t arf = UINT8_C(4);
        const uint16_t old_ar = dependency == 3U ? UINT16_MAX
                                                 : UINT16_C(0x4200);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ld_status_word(target, arf),
            .read_value_word = UINT16_C(0xFFFF),
            .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu =
            init_cpu(&fixture, storage, true, target, arf, old_ar, &status);

        if (dependency == 0U) {
            cpu->arch_state.ar_known_mask = (uint8_t)(
                cpu->arch_state.ar_known_mask & (uint8_t)~(UINT8_C(1) << arf));
        } else if (dependency == 1U) {
            cpu->arch_state.status.st0_known_mask = (uint16_t)(
                cpu->arch_state.status.st0_known_mask &
                (uint16_t)~TI_C54X_ST0_ARP_MASK);
        } else if (dependency == 2U) {
            cpu->arch_state.status.st1_known_mask = (uint16_t)(
                cpu->arch_state.status.st1_known_mask &
                (uint16_t)~TI_C54X_ST1_CMPT_MASK);
        }
        const ti_c54x_arch_state before = cpu->arch_state;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before, sizeof(before)) ==
                           0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
        (void)status;
    }
}

int
main(void)
{
    test_ready_all_arfs_and_values();
    test_wait_resumes_same_transaction_without_refetch();
    test_fault_and_no_callback_roll_back();
    test_unknown_dependencies_and_ar_boundary_reject();
    return 0;
}
