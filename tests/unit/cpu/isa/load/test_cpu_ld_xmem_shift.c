/*
 * Module contract
 *
 * Owns: public-CPU RED proofs for one-word `LD Xmem,SHFT,dst`,
 * including SXM widening with OVM saturation and destination
 * overflow, single X-only D-bus reads with X-commit-only updates
 * for MOD0/MOD1/MOD2, MOD3 indexed/circular commits, resumable
 * completion, fault atomicity, and transactional rejection.
 * Does not own: catalog/decode or pure load effects, address
 * internals, endpoint behavior, production code, timing, or
 * diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/block_size_register.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint16_t
ld_word(uint8_t destination, uint8_t xmod, uint8_t xar_field,
        uint8_t shift)
{
    return (uint16_t)(UINT16_C(0x9400) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (uint16_t)(((uint16_t)(xmod & UINT8_C(3))) << 6U) |
                      (uint16_t)(((uint16_t)(xar_field & UINT8_C(3)))
                                 << 4U) |
                      (uint16_t)(shift & UINT8_C(0x0F)));
}

static ti_c54x_cpu *
init_load(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
          bool with_endpoint, uint8_t arf, uint16_t ar_value,
          uint8_t sxm, uint8_t ovm, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, with_endpoint,
                                            status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    cpu->arch_state.status.st1 = (uint16_t)(
        (sxm != UINT8_C(0) ? TI_C54X_ST1_SXM_MASK : UINT16_C(0)) |
        (ovm != UINT8_C(0) ? TI_C54X_ST1_OVM_MASK : UINT16_C(0)));
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    ti_c54x_cpu_isa_seed_address(cpu, arf, ar_value);
    ti_c54x_cpu_isa_seed_sxm(cpu, sxm);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x1111222233)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000aabbcc)) ==
                       TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

/* Independent oracle mirroring the ops contract. */
static uint64_t
expected_shifted(uint16_t read_value, uint8_t sxm, uint8_t shift,
                 uint8_t ovm, bool *overflow)
{
    uint64_t widened = (uint64_t)read_value;
    uint64_t value;
    uint64_t guard;
    uint8_t bit31;
    bool negative;

    if (sxm != UINT8_C(0) &&
        (read_value & UINT16_C(0x8000)) != UINT16_C(0)) {
        widened |= UINT64_C(0xFFFFFFFF0000);
    }
    negative = (widened & UINT64_C(0x8000000000)) != UINT64_C(0);
    value = (widened << shift) & TI_C54X_U40_MASK;
    guard = (value >> 32U) & UINT64_C(0xFF);
    bit31 = (uint8_t)((value >> 31U) & UINT64_C(1));
    *overflow =
        guard != (bit31 != UINT8_C(0) ? UINT64_C(0xFF) : UINT64_C(0));
    if (*overflow && ovm != UINT8_C(0)) {
        value = negative ? UINT64_C(0xFF80000000)
                         : UINT64_C(0x007FFFFFFF);
    }
    return value;
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
    /* One X read only: no Y transfer is ever issued. */
    TI_C54X_TEST_CHECK(fixture->endpoint_calls == 1U);
}

static void
check_ov(const ti_c54x_cpu *cpu, uint8_t destination, bool overflow)
{
    const uint16_t mask = destination == UINT8_C(0) ? TI_C54X_ST0_OVA_MASK
                                                   : TI_C54X_ST0_OVB_MASK;

    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & mask) ==
                       (overflow ? mask : UINT16_C(0)));
}

static void
test_ready_retires_shift_with_x_commit_only(void)
{
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x7FFF),
                                     UINT16_C(0x8000), UINT16_C(0xFFFF)};
    static const uint8_t shifts[] = {UINT8_C(0), UINT8_C(1),
                                     UINT8_C(15)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
            for (uint8_t ovm = 0U; ovm <= UINT8_C(1); ++ovm) {
                for (uint8_t xar_field = 0U; xar_field <= UINT8_C(3);
                     ++xar_field) {
                    const uint8_t arf = (uint8_t)(xar_field + UINT8_C(2));
                    for (uint8_t xmod = 0U; xmod <= UINT8_C(2); ++xmod) {
                        for (size_t shift_index = 0U;
                             shift_index <
                             sizeof(shifts) / sizeof(shifts[0]);
                             ++shift_index) {
                            for (size_t read_index = 0U;
                                 read_index <
                                 sizeof(reads) / sizeof(reads[0]);
                                 ++read_index) {
                                const uint16_t old_ar = UINT16_C(0x2110);
                                ti_c54x_cpu_isa_memory_fixture fixture = {
                                    .mode =
                                        TI_C54X_CPU_ISA_MEMORY_READY,
                                    .program_word = ld_word(
                                        destination, xmod, xar_field,
                                        shifts[shift_index]),
                                    .read_value_word =
                                        reads[read_index],
                                    .fault_status = TI_C54X_BUS_FAULT};
                                _Alignas(max_align_t) unsigned char storage[1024] = {
                                    0};
                                ti_c54x_error status;
                                ti_c54x_cpu *cpu = init_load(
                                    &fixture, storage, true, arf, old_ar,
                                    sxm, ovm, &status);
                                ti_c54x_run_result result =
                                    ti_c54x_cpu_run(cpu, UINT64_C(1));
                                ti_c54x_state state;
                                bool overflow = false;
                                const uint64_t expected = expected_shifted(
                                    reads[read_index], sxm,
                                    shifts[shift_index], ovm, &overflow);
                                uint16_t expected_ar = old_ar;

                                (void)status;
                                if (xmod == UINT8_C(1)) {
                                    expected_ar =
                                        (uint16_t)(old_ar - UINT16_C(1));
                                } else if (xmod == UINT8_C(2)) {
                                    expected_ar =
                                        (uint16_t)(old_ar + UINT16_C(1));
                                }
                                TI_C54X_TEST_CHECK(
                                    result.reason ==
                                    TI_C54X_STOP_BUDGET_EXHAUSTED);
                                TI_C54X_TEST_CHECK(result.bus_status ==
                                                   TI_C54X_BUS_OK);
                                TI_C54X_TEST_CHECK(
                                    result.instructions_attempted ==
                                    UINT64_C(1));
                                TI_C54X_TEST_CHECK(
                                    result.instructions_retired ==
                                    UINT64_C(1));
                                TI_C54X_TEST_CHECK(result.pc_word ==
                                                   UINT32_C(1));
                                TI_C54X_TEST_CHECK(
                                    result.opcode_word ==
                                    fixture.program_word);
                                TI_C54X_TEST_CHECK(result.opcode_valid);
                                TI_C54X_TEST_CHECK(fixture.program_reads ==
                                                   1U);
                                TI_C54X_TEST_CHECK(fixture.begin_calls ==
                                                   1U);
                                TI_C54X_TEST_CHECK(fixture.ready_calls ==
                                                   1U);
                                check_read_request(&fixture, old_ar);
                                TI_C54X_TEST_CHECK(
                                    cpu->arch_state.ar[arf] ==
                                    expected_ar);
                                check_ov(cpu, destination, overflow);
                                TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(
                                                       cpu, &state) ==
                                                   TI_C54X_ERROR_OK);
                                TI_C54X_TEST_CHECK(state.pc_word ==
                                                   UINT32_C(1));
                                TI_C54X_TEST_CHECK(
                                    state.accumulator_a_raw40 ==
                                    (destination == UINT8_C(0)
                                         ? expected
                                         : UINT64_C(0x1111222233)));
                                TI_C54X_TEST_CHECK(
                                    state.accumulator_b_raw40 ==
                                    (destination == UINT8_C(1)
                                         ? expected
                                         : UINT64_C(0x0000aabbcc)));
                                TI_C54X_TEST_CHECK(
                                    state.instructions_attempted_total ==
                                    UINT64_C(1));
                                TI_C54X_TEST_CHECK(
                                    state.instructions_retired_total ==
                                    UINT64_C(1));
                                TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(
                                                       cpu) ==
                                                   TI_C54X_ERROR_OK);
                            }
                        }
                    }
                }
            }
        }
    }
}

static void
seed_mod3(ti_c54x_cpu *cpu, uint16_t ar0_value, uint16_t bk_value)
{
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state,
                                                   UINT8_C(0),
                                                   ar0_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&cpu->arch_state,
                                                   bk_value) ==
                       TI_C54X_ARCH_STATE_OK);
}

static void
test_mod3_indexed_and_circular_commits(void)
{
    /* BK=0/AR0=0 documents no modification; BK=0/AR0=1 indexes;
     * BK=8 with AR0=1 wraps circularly inside the buffer. */
    static const struct {
        uint16_t ar0_value;
        uint16_t bk_value;
        uint16_t old_ar;
        uint16_t expected_ar;
    } cases[] = {{UINT16_C(0), UINT16_C(0), UINT16_C(0x2110),
                  UINT16_C(0x2110)},
                 {UINT16_C(1), UINT16_C(0), UINT16_C(0x2110),
                  UINT16_C(0x2111)},
                 {UINT16_C(1), UINT16_C(8), UINT16_C(0x2107),
                  UINT16_C(0x2100)}};

    for (size_t case_index = 0U;
         case_index < sizeof(cases) / sizeof(cases[0]); ++case_index) {
        const uint8_t arf = UINT8_C(4);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ld_word(UINT8_C(0), UINT8_C(3), UINT8_C(2),
                                    UINT8_C(0)),
            .read_value_word = UINT16_C(0x1234),
            .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu =
            init_load(&fixture, storage, true, arf, cases[case_index].old_ar,
                      UINT8_C(0), UINT8_C(0), &status);
        ti_c54x_run_result result;

        (void)status;
        seed_mod3(cpu, cases[case_index].ar0_value,
                  cases[case_index].bk_value);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
        check_read_request(&fixture, cases[case_index].old_ar);
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] ==
                           cases[case_index].expected_ar);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_fault_does_not_commit_or_publish(void)
{
    const uint8_t arf = UINT8_C(2);
    const uint16_t old_ar = UINT16_C(0x4000);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_FAULT,
        .program_word = ld_word(UINT8_C(0), UINT8_C(2), UINT8_C(0),
                                UINT8_C(3)),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_load(&fixture, storage, true, arf, old_ar, UINT8_C(1),
                  UINT8_C(0), &status);
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
        const uint8_t arf = UINT8_C(3);
        const uint16_t old_ar = UINT16_C(0x4200);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ld_word(UINT8_C(0), UINT8_C(0), UINT8_C(1),
                                    UINT8_C(0))};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_load(&fixture, storage, false, arf, old_ar,
                                     UINT8_C(0), UINT8_C(0), &status);
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
        /* MOD1 decrement at the zero boundary rejects before issue. */
        const uint8_t arf = UINT8_C(5);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ld_word(UINT8_C(1), UINT8_C(1), UINT8_C(3),
                                    UINT8_C(0))};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_load(&fixture, storage, true, arf,
                                     UINT16_C(0), UINT8_C(0), UINT8_C(0),
                                     &status);
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == UINT16_C(0));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

int
main(void)
{
    test_ready_retires_shift_with_x_commit_only();
    test_mod3_indexed_and_circular_commits();
    test_fault_does_not_commit_or_publish();
    test_no_callback_and_boundary_reject_before_issue();
    return 0;
}
