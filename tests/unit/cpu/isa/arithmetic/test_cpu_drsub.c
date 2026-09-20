/* Module contract
 * Owns: public-CPU proofs for `DRSUB Lmem,src` retirement
 * (reversed subtract Lmem - src), including direct page-0 and
 * indirect MOD1 DATA pair reads with accumulator stores, C-bit
 * rules in both C16 modes, AR
 * post-update, MOD3-read rejection, and unknown-C16
 * transactionality.
 * Does not own: catalog/decode or pure arithmetic effects,
 * address-planner internals, endpoint reducer behavior,
 * production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint16_t
drsub_word(uint8_t source, uint8_t indirect, uint8_t cookie)
{
    return (uint16_t)(UINT16_C(0x5800) |
                      ((uint16_t)(source & UINT8_C(1)) << 8U) |
                      ((uint16_t)(indirect & UINT8_C(1)) << 7U) |
                      (uint16_t)(cookie & UINT8_C(0x7F)));
}

static ti_c54x_cpu *
init_drsub(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
          uint64_t acc_a, uint64_t acc_b, uint16_t st1_set_mask,
          ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, true, status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    /* Fresh-CPU status bits are unknown; seed SXM/OVM/C16 from the
     * caller mask over the reset ST1. */
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 | st1_set_mask);
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_SXM_MASK | TI_C54X_ST1_OVM_MASK |
                   TI_C54X_ST1_C16_MASK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           acc_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           acc_b) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static uint8_t
read_ovb(const ti_c54x_cpu *cpu)
{
    const ti_c54x_status_register_field_result ovb =
        ti_c54x_status_register_read_ovb(&cpu->arch_state.status);

    TI_C54X_TEST_CHECK(ovb.status == TI_C54X_STATUS_REGISTER_OK);
    return (uint8_t)ovb.raw_value;
}

static void
test_direct_c16_clear_retires(void)
{
    /* DRSUB dma,A with A=00 0000 1000, Lmem=1000 1000 (both
     * reads return 0x1000): A=00 1000 0000 with C=1 (no borrow;
     * reversed Lmem - src). */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = drsub_word(UINT8_C(0), UINT8_C(0), UINT8_C(0x0A)),
        .read_value_word = UINT16_C(0x1000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_drsub(&fixture, storage, UINT64_C(0x0000001000),
                  UINT64_C(0), UINT16_C(0), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == UINT16_C(0x000A));
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == UINT16_C(0x000B));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0x0010000000));
    TI_C54X_TEST_CHECK(state.c == UINT8_C(1));
    TI_C54X_TEST_CHECK(state.c_known == true);
    TI_C54X_TEST_CHECK(read_ovb(cpu) == UINT8_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_direct_c16_set_retires(void)
{
    /* Dual mode with Lmem=00FF 00FF and A=00 1122 3344:
     * hi 00FF-1122=EFDD, lo 00FF-3344=CDBB, C=0 (borrows). */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = drsub_word(UINT8_C(0), UINT8_C(0), UINT8_C(0x0B)),
        .read_value_word = UINT16_C(0x00FF),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_drsub(&fixture, storage, UINT64_C(0x0011223344),
                  UINT64_C(0), TI_C54X_ST1_C16_MASK, &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0xFFEFDDCDBB));
    TI_C54X_TEST_CHECK(state.c == UINT8_C(0));
    TI_C54X_TEST_CHECK(read_ovb(cpu) == UINT8_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_indirect_mod1_retires(void)
{
    /* DRSUB *AR2-,A with AR2=0x0100 and Lmem=2000 2000,
     * A=0: A=00 2000 2000 with AR2 post-decremented by two. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = drsub_word(UINT8_C(0), UINT8_C(1), UINT8_C(0x0A)),
        .read_value_word = UINT16_C(0x2000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_drsub(&fixture, storage, UINT64_C(0),
                  UINT64_C(0), UINT16_C(0), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(2), UINT16_C(0x0100));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == UINT16_C(0x0101));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == UINT16_C(0x00FE));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0x0020002000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_mod3_read_rejects(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = drsub_word(UINT8_C(0), UINT8_C(1), UINT8_C(0x1A)),
        .read_value_word = UINT16_C(0x2000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_drsub(&fixture, storage, UINT64_C(0), UINT64_C(0),
                  UINT16_C(0), &status);
    ti_c54x_run_result result;

    (void)status;
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(2), UINT16_C(0x0100));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_c16_rejects(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = drsub_word(UINT8_C(0), UINT8_C(0), UINT8_C(0x0A)),
        .read_value_word = UINT16_C(0x1000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_drsub(&fixture, storage, UINT64_C(0x0056788933),
                  UINT64_C(0), UINT16_C(0), &status);
    ti_c54x_run_result result;

    (void)status;
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask &
                   (uint16_t)~TI_C54X_ST1_C16_MASK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_direct_c16_clear_retires();
    test_direct_c16_set_retires();
    test_indirect_mod1_retires();
    test_mod3_read_rejects();
    test_unknown_c16_rejects();
    return 0;
}
