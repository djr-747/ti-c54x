/* Module contract
 * Owns: public-CPU RED proofs for the `CMPS src,Smem` direct page-0
 * slice, including max-select publication, the TRN shift with the
 * taken bit, TC publication, and transactional rejection on
 * unknown TRN input. Page-0 direct observes no DP state.
 * Does not own: catalog/decode or pure store effects, address-planner
 * internals, endpoint reducer behavior, production code, timing, or
 * diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/mmr_file.h"
#include "../../../../../src/state/status/registers.h"

#include <stddef.h>
#include <stdint.h>

static uint16_t
cmps_direct_word(uint8_t select, uint8_t dma)
{
    uint16_t word =
        (uint16_t)(UINT16_C(0x8E00) | (uint16_t)(dma & UINT8_C(0x7F)));

    if (select != UINT8_C(0)) {
        word |= UINT16_C(0x0100);
    }
    return word;
}

static ti_c54x_cpu *
init_cmps(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
          uint8_t select, uint64_t acc_a, uint64_t acc_b, uint16_t trn,
          ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, true, status);

    (void)select;
    /* Page-0 direct seeds no address registers. */
    (void)ti_c54x_cpu_isa_seed_address;
    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           acc_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           acc_b) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_write_trn(&cpu->arch_state.mmr_file,
                                                 trn) ==
                       TI_C54X_MMR_FILE_OK);
    return cpu;
}

static void
check_retired_write(ti_c54x_cpu *cpu,
                    const ti_c54x_cpu_isa_memory_fixture *fixture,
                    uint8_t dma, uint16_t expected_value, uint16_t expected_trn,
                    uint8_t expected_tc)
{
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    uint16_t trn_word = UINT16_C(0);

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture->endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture->requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word == dma);
    TI_C54X_TEST_CHECK(fixture->requests[0].write_value_word ==
                       expected_value);
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_read_trn(&cpu->arch_state.mmr_file,
                                                &trn_word) ==
                       TI_C54X_MMR_FILE_OK);
    TI_C54X_TEST_CHECK(trn_word == expected_trn);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_tc(
                           &cpu->arch_state.status).raw_value == expected_tc);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_taken_stores_high_and_shifts_trn(void)
{
    /* A hi 0x0070 beats lo 0x0012: stores 0x0070, shifts
     * (0x1234 << 1) | 0 into TRN with TC clear. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = cmps_direct_word(UINT8_C(0), UINT8_C(0x20)),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_cmps(&fixture, storage, UINT8_C(0), UINT64_C(0x0000700012),
                  UINT64_C(0), UINT16_C(0x1234), &status);

    (void)status;
    check_retired_write(cpu, &fixture, UINT8_C(0x20), UINT16_C(0x0070),
                        UINT16_C(0x2468), UINT8_C(0));
}

static void
test_untaken_stores_low_and_sets_tc(void)
{
    /* A hi 0x0012 loses to lo 0x0070: stores 0x0070, shifts
     * (0x1234 << 1) | 1 into TRN with TC set. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = cmps_direct_word(UINT8_C(0), UINT8_C(0x20)),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_cmps(&fixture, storage, UINT8_C(0), UINT64_C(0x0000120070),
                  UINT64_C(0), UINT16_C(0x1234), &status);

    (void)status;
    check_retired_write(cpu, &fixture, UINT8_C(0x20), UINT16_C(0x0070),
                        UINT16_C(0x2469), UINT8_C(1));
}

static void
test_b_source_select_retires(void)
{
    /* S=1 stores from B: B hi 0x4321 beats lo 0x1234. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = cmps_direct_word(UINT8_C(1), UINT8_C(0x21)),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_cmps(&fixture, storage, UINT8_C(1), UINT64_C(0),
                  UINT64_C(0x0043211234), UINT16_C(0xFFFF), &status);

    (void)status;
    check_retired_write(cpu, &fixture, UINT8_C(0x21), UINT16_C(0x4321),
                        UINT16_C(0xFFFE), UINT8_C(0));
}

static void
test_unknown_trn_rejects_before_issue(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = cmps_direct_word(UINT8_C(0), UINT8_C(0x20)),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_cmps(&fixture, storage, UINT8_C(0), UINT64_C(0x0000700012),
                  UINT64_C(0), UINT16_C(0x1234), &status);
    const ti_c54x_arch_state before_arch = cpu->arch_state;
    ti_c54x_run_result result;

    (void)status;
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_reset(&cpu->arch_state.mmr_file) ==
                       TI_C54X_MMR_FILE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == before_arch.pc_word);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_taken_stores_high_and_shifts_trn();
    test_untaken_stores_low_and_sets_tc();
    test_b_source_select_retires();
    test_unknown_trn_rejects_before_issue();
    return 0;
}
