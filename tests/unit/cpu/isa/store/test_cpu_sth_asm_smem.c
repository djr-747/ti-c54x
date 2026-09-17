/* Module contract
 * Owns: public-CPU RED proofs for `STH src,ASM,Smem` fixed-MOD2 stores,
 * including ASM-shifted high-word publication, SXM gating, TI worked
 * vectors, and transactional rejection on unknown ASM/SXM/accumulator.
 * Does not own: catalog/decode or pure store effects, address-planner
 * internals, endpoint reducer behavior, production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/status/registers.h"

#include <stddef.h>
#include <stdint.h>

static uint16_t
sth_word(uint8_t source, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x8690) |
                      ((uint16_t)(source & UINT8_C(1)) << 8U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_cpu *
init_store(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
           bool with_endpoint, uint8_t arf, uint16_t ar_value,
           uint8_t source, uint64_t source_value, uint64_t other_value,
           uint8_t asm_value, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, with_endpoint,
                                            status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, arf, ar_value);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_asm(
                           &cpu->arch_state.status,
                           asm_value) == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           source == UINT8_C(0) ? TI_C54X_ACCUMULATOR_A
                                                : TI_C54X_ACCUMULATOR_B,
                           source_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           source == UINT8_C(0) ? TI_C54X_ACCUMULATOR_B
                                                : TI_C54X_ACCUMULATOR_A,
                           other_value) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void
check_ready_case(uint8_t source, uint8_t arf, uint16_t old_ar,
                 uint64_t source_value, uint64_t other_value,
                 uint8_t asm_value, uint16_t expected)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = sth_word(source, arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_store(&fixture, storage, true, arf, old_ar, source,
                   source_value, other_value, asm_value, &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == old_ar);
    TI_C54X_TEST_CHECK(fixture.requests[0].write_value_word == expected);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + UINT16_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_shift_points(void)
{
    /* ASM=0 identity through both selects and SXM readings. */
    check_ready_case(UINT8_C(0), UINT8_C(2), UINT16_C(0x2110),
                     UINT64_C(0x0012345678), UINT64_C(0xAAAAAAAA),
                     UINT8_C(0), UINT16_C(0x1234));
    check_ready_case(UINT8_C(1), UINT8_C(5), UINT16_C(0x2110),
                     UINT64_C(0xFF87654321), UINT64_C(0x5555555555),
                     UINT8_C(0), UINT16_C(0x8765));
    /* Positive, negative, and extreme ASM counts. */
    check_ready_case(UINT8_C(0), UINT8_C(2), UINT16_C(0x2110),
                     UINT64_C(0x0012345678), UINT64_C(0),
                     UINT8_C(1), UINT16_C(0x2468));
    /* ASM=15 left shift drops the top guard nibble: (0xFF87654321 << 15)
     * mod 2^40 is 0xB2A1908000, bits 31-16 are 0xA190. */
    check_ready_case(UINT8_C(1), UINT8_C(2), UINT16_C(0x2110),
                     UINT64_C(0xFF87654321), UINT64_C(0),
                     UINT8_C(15), UINT16_C(0xA190));
    check_ready_case(UINT8_C(0), UINT8_C(2), UINT16_C(0x2110),
                     UINT64_C(0x0012345678), UINT64_C(0),
                     UINT8_C(16), UINT16_C(0x0000));
    check_ready_case(UINT8_C(1), UINT8_C(2), UINT16_C(0x2110),
                     UINT64_C(0xFF87654321), UINT64_C(0),
                     UINT8_C(16), UINT16_C(0xFFFF));
    check_ready_case(UINT8_C(0), UINT8_C(2), UINT16_C(0x2110),
                     UINT64_C(0x0012345678), UINT64_C(0),
                     UINT8_C(24), UINT16_C(0x0012));
    check_ready_case(UINT8_C(1), UINT8_C(2), UINT16_C(0x2110),
                     UINT64_C(0xFF87654321), UINT64_C(0),
                     UINT8_C(24), UINT16_C(0xFF87));
    check_ready_case(UINT8_C(0), UINT8_C(2), UINT16_C(0x2110),
                     UINT64_C(0x0012345678), UINT64_C(0),
                     UINT8_C(28), UINT16_C(0x0123));
    check_ready_case(UINT8_C(0), UINT8_C(2), UINT16_C(0x2110),
                     UINT64_C(0x0012345678), UINT64_C(0),
                     UINT8_C(31), UINT16_C(0x091A));
    check_ready_case(UINT8_C(1), UINT8_C(2), UINT16_C(0x2110),
                     UINT64_C(0xFF87654321), UINT64_C(0),
                     UINT8_C(31), UINT16_C(0xC3B2));
}

static void
test_ti_vectors(void)
{
    /* TI SPRU172C STH B,-8 example: B FF 8421 1234 stores FF84. */
    check_ready_case(UINT8_C(1), UINT8_C(7), UINT16_C(0x0321),
                     UINT64_C(0xFF84211234), UINT64_C(0),
                     UINT8_C(24), UINT16_C(0xFF84));
    check_ready_case(UINT8_C(1), UINT8_C(7), UINT16_C(0x0321),
                     UINT64_C(0xFF84211234), UINT64_C(0),
                     UINT8_C(24), UINT16_C(0xFF84));
    /* TI SPRU172C STH A,-4 example (SXM=1): A FF 8421 1234 stores F842. */
    check_ready_case(UINT8_C(0), UINT8_C(0), UINT16_C(0x020A),
                     UINT64_C(0xFF84211234), UINT64_C(0),
                     UINT8_C(28), UINT16_C(0xF842));
}

static void
test_wait_resumes_and_publishes_shifted(void)
{
    const uint8_t arf = UINT8_C(4);
    const uint16_t old_ar = UINT16_C(0x3200);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = sth_word(UINT8_C(0), arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_store(&fixture, storage, true, arf, old_ar, UINT8_C(0),
                   UINT64_C(0x0012345678), UINT64_C(0),
                   UINT8_C(1), &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(fixture.requests[1].write_value_word ==
                       UINT16_C(0x2468));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + UINT16_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_asm_rejects(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = sth_word(UINT8_C(0), UINT8_C(2)),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_store(&fixture, storage, true, UINT8_C(2), UINT16_C(0x2110),
                   UINT8_C(0), UINT64_C(0x0012345678), UINT64_C(0),
                   UINT8_C(0), &status);
    ti_c54x_run_result result;

    (void)status;
    /* Reset leaves ST1.ASM unknown: clear the field the seeder set. */
    cpu->arch_state.status.st1_known_mask = (uint16_t)(
        cpu->arch_state.status.st1_known_mask &
        (uint16_t)~TI_C54X_ST1_ASM_MASK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_shift_points();
    test_ti_vectors();
    test_wait_resumes_and_publishes_shifted();
    test_unknown_asm_rejects();
    return 0;
}
