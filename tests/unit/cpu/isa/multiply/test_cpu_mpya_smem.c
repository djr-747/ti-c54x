/* Module contract
 * Owns: public-CPU RED proofs for `MPYA Smem` retirement, including
 * the TI Example 1 indirect read with B store plus the read-phase T
 * load, and a direct page-0 read with FRCT scaling.
 * Does not own: catalog/decode or pure multiply effects,
 * address-planner internals, endpoint reducer behavior, production
 * code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/foundation/arithmetic40.h"
#include "../../../../../src/state/register_layout.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static ti_c54x_cpu *
init_mpya(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
          uint16_t t_value, uint64_t acc_a, uint64_t acc_b,
          ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, true, status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    cpu->t_register.raw_value = t_value;
    cpu->t_register.known = true;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           acc_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           acc_b) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void
test_indirect_example1_retires(void)
{
    /* TI MPYA Example 1: MPYA *AR2 with A=FF 8765 1111, T=0x1234,
     * mem[0x0200]=0x5678, FRCT=0: B=FF D743 6558, T=0x5678. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x3182),
        .read_value_word = UINT16_C(0x5678),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_mpya(&fixture, storage, UINT16_C(0x1234),
                  UINT64_C(0xFF87651111), UINT64_C(0x320), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(2), UINT16_C(0x0200));
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
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == UINT16_C(0x0200));
    /* MOD0 leaves AR2 untouched. */
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[2] == UINT16_C(0x0200));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0xFF87651111));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 ==
                       UINT64_C(0xFFD7436558));
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x5678));
    TI_C54X_TEST_CHECK(cpu->t_register.known == true);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_direct_frct_retires(void)
{
    /* 0x3134: I=0, addr=0x34: A-high=1 times Smem=2 with FRCT=1
     * stores 4 into B and loads T with the read value. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x3134),
        .read_value_word = UINT16_C(0x0002),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_mpya(&fixture, storage, UINT16_C(0x1234),
                  UINT64_C(0x00010000), UINT64_C(0), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_FRCT_MASK);
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_FRCT_MASK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == UINT64_C(4));
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x0002));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_indirect_example1_retires();
    test_direct_frct_retires();
    return 0;
}
