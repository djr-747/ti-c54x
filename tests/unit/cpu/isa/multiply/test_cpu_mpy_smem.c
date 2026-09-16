/* Module contract
 * Owns: public-CPU RED proofs for `MPY Smem` retirement, including
 * direct page-0 reads without accumulation, standard-indirect MOD2
 * reads with AR post-increment, B-destination rounding, and
 * accumulator plus overflow commit.
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
init_mpy(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
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
test_direct_page0_retires(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x2034),
        .read_value_word = UINT16_C(0x1234),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_mpy(&fixture, storage, UINT16_C(0x0400), UINT64_C(0x1000),
                 UINT64_C(0), &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    ti_c54x_state state;

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == UINT16_C(0x34));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    /* MPY stores 0x1234 * 0x0400 without adding A=0x1000. */
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x48D000));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == UINT64_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_indirect_mod2_postincrements(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x2095),
        .read_value_word = UINT16_C(0x0002),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_mpy(&fixture, storage, UINT16_C(0x0400), UINT64_C(0),
                 UINT64_C(0), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(5), UINT16_C(0x2110));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word ==
                       UINT16_C(0x2110));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[5] == UINT16_C(0x2111));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    /* 0x0002 * 0x0400 stored into A. */
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x800));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_rounded_b_destination(void)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        /* 0x2320: R=1, S=1, I=0, addr=0x20 (DP0 RAM, not the MMR file). */
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x2320),
        .read_value_word = UINT16_C(0x0002),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_mpy(&fixture, storage, UINT16_C(0x4000), UINT64_C(0),
                 UINT64_C(0), &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    ti_c54x_state state;

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    /* 0x0002 * 0x4000 = 0x8000, rounded to 0x10000, into B. */
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == UINT64_C(0x10000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_direct_page0_retires();
    test_indirect_mod2_postincrements();
    test_rounded_b_destination();
    return 0;
}
