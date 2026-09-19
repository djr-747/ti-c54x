/* Module contract
 * Owns: public-CPU RED proofs for `STL src,SHFT,Xmem` fixed stores,
 * including left-shifted low-word publication over Xmod/Xar
 * selection, the TI syntax-3 worked vector, and transactional
 * rejection on unknown accumulator input.
 * Does not own: catalog/decode or pure store effects, address-planner
 * internals, endpoint reducer behavior, production code, timing, or
 * diagnostics.
 */

#include "../fixture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint16_t
stl_word(uint8_t source, uint8_t xmod, uint8_t xar_field, uint8_t shift)
{
    return (uint16_t)(UINT16_C(0x9800) |
                      ((uint16_t)(source & UINT8_C(1)) << 8U) |
                      ((uint16_t)(xmod & UINT8_C(3)) << 6U) |
                      ((uint16_t)(xar_field & UINT8_C(3)) << 4U) |
                      (uint16_t)(shift & UINT8_C(0x0F)));
}

static ti_c54x_cpu *
init_store(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
           bool with_endpoint, uint8_t arf, uint16_t ar_value,
           uint8_t source, uint64_t source_value, uint64_t other_value,
           ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, with_endpoint,
                                            status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, arf, ar_value);
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
check_ready_case(uint8_t source, uint8_t xmod, uint8_t xar_field,
                 uint16_t old_ar, uint64_t source_value,
                 uint64_t other_value, uint8_t shift, uint16_t expected)
{
    const uint8_t arf = (uint8_t)(xar_field + UINT8_C(2));
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = stl_word(source, xmod, xar_field, shift),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_store(&fixture, storage, true, arf, old_ar, source,
                   source_value, other_value, &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    uint16_t expected_ar = old_ar;

    (void)status;
    if (xmod == UINT8_C(1)) {
        expected_ar = (uint16_t)(old_ar - UINT16_C(1));
    } else if (xmod == UINT8_C(2)) {
        expected_ar = (uint16_t)(old_ar + UINT16_C(1));
    }
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
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == expected_ar);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_shift_points(void)
{
    /* SHFT=0 stores the raw low word; SHFT=8 shifts left 8; SHFT=15
     * keeps only bit 0 at bit 15. SXM is not seeded: the stored
     * field never observes it. */
    for (uint8_t xmod = 0U; xmod <= UINT8_C(2); ++xmod) {
        for (uint8_t xar_field = 0U; xar_field <= UINT8_C(3);
             ++xar_field) {
            check_ready_case(UINT8_C(0), xmod, xar_field,
                             UINT16_C(0x2110), UINT64_C(0x0012345678),
                             UINT64_C(0xAAAAAAAA), UINT8_C(0),
                             UINT16_C(0x5678));
            check_ready_case(UINT8_C(1), xmod, xar_field,
                             UINT16_C(0x2110), UINT64_C(0xFF87654321),
                             UINT64_C(0x5555555555), UINT8_C(0),
                             UINT16_C(0x4321));
            check_ready_case(UINT8_C(0), xmod, xar_field,
                             UINT16_C(0x2110), UINT64_C(0x0012345678),
                             UINT64_C(0), UINT8_C(8),
                             UINT16_C(0x7800));
            check_ready_case(UINT8_C(1), xmod, xar_field,
                             UINT16_C(0x2110), UINT64_C(0xFF87654321),
                             UINT64_C(0), UINT8_C(8),
                             UINT16_C(0x2100));
            check_ready_case(UINT8_C(0), xmod, xar_field,
                             UINT16_C(0x2110), UINT64_C(0x0012345679),
                             UINT64_C(0), UINT8_C(15),
                             UINT16_C(0x8000));
            check_ready_case(UINT8_C(1), xmod, xar_field,
                             UINT16_C(0x2110), UINT64_C(0xFF87654321),
                             UINT64_C(0), UINT8_C(15),
                             UINT16_C(0x8000));
        }
    }
}

static void
test_ti_vector(void)
{
    /* TI SPRU172C STL syntax-3 example: STL A,7 with
     * A FF 8421 1234 stores 1A00. */
    check_ready_case(UINT8_C(0), UINT8_C(1), UINT8_C(0),
                     UINT16_C(0x020B), UINT64_C(0xFF84211234),
                     UINT64_C(0), UINT8_C(7), UINT16_C(0x1A00));
}

static void
test_wait_resumes_and_publishes_shifted(void)
{
    const uint8_t arf = UINT8_C(4);
    const uint16_t old_ar = UINT16_C(0x3200);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = stl_word(UINT8_C(0), UINT8_C(2), UINT8_C(2),
                                 UINT8_C(8)),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_store(&fixture, storage, true, arf, old_ar, UINT8_C(0),
                   UINT64_C(0x0012345678), UINT64_C(0), &status);
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
                       UINT16_C(0x7800));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + UINT16_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_accumulator_rejects(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = stl_word(UINT8_C(0), UINT8_C(0), UINT8_C(0),
                                 UINT8_C(8)),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_store(&fixture, storage, true, UINT8_C(2), UINT16_C(0x2110),
                   UINT8_C(0), UINT64_C(0x0012345678), UINT64_C(0),
                   &status);
    ti_c54x_run_result result;

    (void)status;
    /* Reset leaves accumulator A unknown: clear its knownness bit. */
    cpu->accumulators.known_mask =
        (uint8_t)(cpu->accumulators.known_mask & (uint8_t)~UINT8_C(1));
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
    test_ti_vector();
    test_wait_resumes_and_publishes_shifted();
    test_unknown_accumulator_rejects();
    return 0;
}
