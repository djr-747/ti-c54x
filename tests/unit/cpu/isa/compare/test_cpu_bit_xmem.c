/*
 * Module contract
 *
 * Owns: public-CPU RED proofs for one-word `BIT Xmem,BITC`,
 * including TC publication over all sixteen BITC codes with the
 * addressed bit set and clear, single X-only D-bus reads with
 * X-commit-only updates for MOD0/MOD1/MOD2, MOD3 indexed and
 * circular commits, resumable completion, fault atomicity, and
 * transactional rejection.
 * Does not own: catalog/decode or pure BIT effects, address
 * internals, endpoint behavior, production code, timing, or
 * diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/block_size_register.h"
#include "../../../../../src/state/register_layout.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint16_t
bit_word(uint8_t xmod, uint8_t xar_field, uint8_t bitc)
{
    return (uint16_t)(UINT16_C(0x9600) |
                      (uint16_t)(((uint16_t)(xmod & UINT8_C(3))) << 6U) |
                      (uint16_t)(((uint16_t)(xar_field & UINT8_C(3)))
                                 << 4U) |
                      (uint16_t)(bitc & UINT8_C(0x0F)));
}

static ti_c54x_cpu *
init_bit(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
         bool with_endpoint, uint8_t arf, uint16_t ar_value,
         ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, with_endpoint,
                                            status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, arf, ar_value);
    cpu->arch_state.status.st0 = UINT16_C(0xA5C3);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    return cpu;
}

/* Independent oracle mirroring the ops contract. */
static uint8_t
expected_tc(uint16_t read_value, uint8_t bitc)
{
    return (uint8_t)(((read_value >> (15U - bitc)) & UINT16_C(1)) !=
                         UINT16_C(0)
                     ? UINT8_C(1)
                     : UINT8_C(0));
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
check_tc(const ti_c54x_cpu *cpu, uint16_t before_st0, uint8_t tc)
{
    const uint16_t expected =
        (uint16_t)((before_st0 & (uint16_t)~TI_C54X_ST0_TC_MASK) |
                   (tc != UINT8_C(0) ? TI_C54X_ST0_TC_MASK
                                     : UINT16_C(0)));

    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == expected);
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0_known_mask &
                        TI_C54X_ST0_TC_MASK) != UINT16_C(0));
}

static void
test_ready_retires_bit_with_x_commit_only(void)
{
    static const uint16_t reads[] = {UINT16_C(0x0000), UINT16_C(0x8000),
                                     UINT16_C(0x0001), UINT16_C(0xFFFF)};
    static const uint8_t bitcs[] = {UINT8_C(0), UINT8_C(7),
                                    UINT8_C(15)};

    for (uint8_t xar_field = 0U; xar_field <= UINT8_C(3); ++xar_field) {
        const uint8_t arf = (uint8_t)(xar_field + UINT8_C(2));
        for (uint8_t xmod = 0U; xmod <= UINT8_C(2); ++xmod) {
            for (size_t bitc_index = 0U;
                 bitc_index < sizeof(bitcs) / sizeof(bitcs[0]);
                 ++bitc_index) {
                for (size_t read_index = 0U;
                     read_index < sizeof(reads) / sizeof(reads[0]);
                     ++read_index) {
                    const uint16_t old_ar = UINT16_C(0x2110);
                    const uint8_t bitc = bitcs[bitc_index];
                    ti_c54x_cpu_isa_memory_fixture fixture = {
                        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
                        .program_word =
                            bit_word(xmod, xar_field, bitc),
                        .read_value_word = reads[read_index],
                        .fault_status = TI_C54X_BUS_FAULT};
                    _Alignas(max_align_t) unsigned char storage[1024] = {
                        0};
                    ti_c54x_error status;
                    ti_c54x_cpu *cpu = init_bit(
                        &fixture, storage, true, arf, old_ar, &status);
                    const uint64_t before_a =
                        cpu->accumulators.a_raw40;
                    const uint64_t before_b =
                        cpu->accumulators.b_raw40;
                    const uint8_t tc =
                        expected_tc(reads[read_index], bitc);
                    ti_c54x_run_result result;
                    uint16_t expected_ar = old_ar;

                    (void)status;
                    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
                    if (xmod == UINT8_C(1)) {
                        expected_ar =
                            (uint16_t)(old_ar - UINT16_C(1));
                    } else if (xmod == UINT8_C(2)) {
                        expected_ar =
                            (uint16_t)(old_ar + UINT16_C(1));
                    }
                    TI_C54X_TEST_CHECK(
                        result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
                    TI_C54X_TEST_CHECK(result.bus_status ==
                                       TI_C54X_BUS_OK);
                    TI_C54X_TEST_CHECK(
                        result.instructions_attempted == UINT64_C(1));
                    TI_C54X_TEST_CHECK(
                        result.instructions_retired == UINT64_C(1));
                    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
                    TI_C54X_TEST_CHECK(
                        result.opcode_word == fixture.program_word);
                    TI_C54X_TEST_CHECK(result.opcode_valid);
                    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
                    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
                    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
                    check_read_request(&fixture, old_ar);
                    TI_C54X_TEST_CHECK(
                        cpu->arch_state.ar[arf] == expected_ar);
                    check_tc(cpu, UINT16_C(0xA5C3), tc);
                    /* BIT publishes TC only: accumulators hold. */
                    TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                                       before_a);
                    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                                       before_b);
                    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                                       TI_C54X_ERROR_OK);
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
            .program_word = bit_word(UINT8_C(3), UINT8_C(2),
                                     UINT8_C(0)),
            .read_value_word = UINT16_C(0x1234),
            .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu =
            init_bit(&fixture, storage, true, arf,
                     cases[case_index].old_ar, &status);
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
        /* BITC 0 addresses bit 15: 0x1234 holds TC 0. */
        check_tc(cpu, UINT16_C(0xA5C3), UINT8_C(0));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                           TI_C54X_ERROR_OK);
    }
}

static void
test_fault_does_not_commit_or_publish(void)
{
    const uint8_t arf = UINT8_C(2);
    const uint16_t old_ar = UINT16_C(0x4000);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_FAULT,
        .program_word = bit_word(UINT8_C(2), UINT8_C(0), UINT8_C(3)),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_bit(&fixture, storage, true, arf, old_ar, &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == UINT16_C(0xA5C3));
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
            .program_word = bit_word(UINT8_C(0), UINT8_C(1),
                                     UINT8_C(0))};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_bit(&fixture, storage, false, arf,
                                    old_ar, &status);
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
            .program_word = bit_word(UINT8_C(1), UINT8_C(3),
                                     UINT8_C(0))};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_bit(&fixture, storage, true, arf,
                                    UINT16_C(0), &status);
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

static void
test_unknown_selected_ar_rejects_transactionally(void)
{
    /* CMPT/ARP seed known through a non-selected AR; the selected
     * AR5 stays reset-unknown, so the resolver gates UNKNOWN_INPUT
     * and nothing commits. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = bit_word(UINT8_C(2), UINT8_C(3), UINT8_C(5)),
        .read_value_word = UINT16_C(0xFFFF),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    cpu = ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(2), UINT16_C(0x1234));
    cpu->arch_state.status.st0 = UINT16_C(0xA5C3);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == UINT16_C(0xA5C3));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_ready_retires_bit_with_x_commit_only();
    test_mod3_indexed_and_circular_commits();
    test_fault_does_not_commit_or_publish();
    test_no_callback_and_boundary_reject_before_issue();
    test_unknown_selected_ar_rejects_transactionally();
    return 0;
}
