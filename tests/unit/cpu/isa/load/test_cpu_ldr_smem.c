/*
 * Module contract
 *
 * Owns: public-CPU RED proofs for one-word `LDR Smem,dst`,
 * including SXM widening with the 0x8000 rounding constant,
 * single-indirect D-bus reads with AR update commits for
 * MOD0/MOD1/MOD2, resumable completion, fault atomicity, and
 * transactional rejection.
 * Does not own: catalog/decode or pure load effects, address
 * internals, endpoint behavior, production code, timing, or
 * diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../../src/state/register_layout.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint16_t
ldr_word(uint8_t destination, uint8_t mode, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x1680) |
                      (uint16_t)(((uint16_t)(destination & UINT8_C(1)))
                                 << 8U) |
                      (uint16_t)(((uint16_t)(mode & UINT8_C(3))) << 3U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_cpu *
init_load(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
          bool with_endpoint, uint8_t arf, uint16_t ar_value,
          uint8_t sxm, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, with_endpoint,
                                            status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
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
expected_rounded(uint16_t read_value, uint8_t sxm)
{
    uint64_t widened = (uint64_t)read_value;

    if (sxm != UINT8_C(0) &&
        (read_value & UINT16_C(0x8000)) != UINT16_C(0)) {
        widened |= UINT64_C(0xFFFFFFFF0000);
    }
    return ((widened << 16U) + UINT64_C(0x8000)) & TI_C54X_U40_MASK;
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
    TI_C54X_TEST_CHECK(fixture->endpoint_calls == 1U);
}

static void
check_no_overflow(const ti_c54x_cpu *cpu)
{
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 &
                        (uint16_t)(TI_C54X_ST0_OVA_MASK |
                                   TI_C54X_ST0_OVB_MASK)) == UINT16_C(0));
}

static void
test_ready_retires_rounding(void)
{
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x0001),
                                     UINT16_C(0x7FFF), UINT16_C(0x8000),
                                     UINT16_C(0xFEDC), UINT16_C(0xFFFF)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1);
         ++destination) {
        for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
            for (uint8_t arf = 0U; arf <= UINT8_C(7); ++arf) {
                for (uint8_t mode = 0U; mode <= UINT8_C(2); ++mode) {
                    for (size_t read_index = 0U;
                         read_index < sizeof(reads) / sizeof(reads[0]);
                         ++read_index) {
                        const uint16_t old_ar = UINT16_C(0x2110);
                        ti_c54x_cpu_isa_memory_fixture fixture = {
                            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
                            .program_word =
                                ldr_word(destination, mode, arf),
                            .read_value_word = reads[read_index],
                            .fault_status = TI_C54X_BUS_FAULT};
                        _Alignas(max_align_t) unsigned char storage[1024] = {
                            0};
                        ti_c54x_error status;
                        ti_c54x_cpu *cpu =
                            init_load(&fixture, storage, true, arf, old_ar,
                                      sxm, &status);
                        ti_c54x_run_result result =
                            ti_c54x_cpu_run(cpu, UINT64_C(1));
                        ti_c54x_state state;
                        const uint64_t expected = expected_rounded(
                            reads[read_index], sxm);
                        uint16_t expected_ar = old_ar;

                        (void)status;
                        if (mode == UINT8_C(1)) {
                            expected_ar =
                                (uint16_t)(old_ar - UINT16_C(1));
                        } else if (mode == UINT8_C(2)) {
                            expected_ar =
                                (uint16_t)(old_ar + UINT16_C(1));
                        }
                        TI_C54X_TEST_CHECK(result.reason ==
                                           TI_C54X_STOP_BUDGET_EXHAUSTED);
                        TI_C54X_TEST_CHECK(result.bus_status ==
                                           TI_C54X_BUS_OK);
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
                                           expected_ar);
                        check_no_overflow(cpu);
                        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu,
                                                              &state) ==
                                           TI_C54X_ERROR_OK);
                        TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(1));
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
                        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                                           TI_C54X_ERROR_OK);
                    }
                }
            }
        }
    }
}

static void
test_fault_does_not_commit_or_publish(void)
{
    const uint8_t arf = UINT8_C(2);
    const uint16_t old_ar = UINT16_C(0x4000);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_FAULT,
        .program_word = ldr_word(UINT8_C(0), UINT8_C(2), arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_load(&fixture, storage, true, arf, old_ar, UINT8_C(1),
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
test_no_callback_and_boundary_reject_before_issue(void)
{
    {
        const uint8_t arf = UINT8_C(3);
        const uint16_t old_ar = UINT16_C(0x4200);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ldr_word(UINT8_C(0), UINT8_C(0), arf)};
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
        /* MOD1 decrement at the zero boundary rejects before issue. */
        const uint8_t arf = UINT8_C(5);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ldr_word(UINT8_C(1), UINT8_C(1), arf)};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu =
            init_load(&fixture, storage, true, arf, UINT16_C(0),
                      UINT8_C(0), &status);
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
    test_ready_retires_rounding();
    test_fault_does_not_commit_or_publish();
    test_no_callback_and_boundary_reject_before_issue();
    return 0;
}
