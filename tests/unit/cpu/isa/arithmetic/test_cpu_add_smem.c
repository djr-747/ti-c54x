/* Module contract
 * Owns: public-CPU RED proofs for `ADD Smem` syntaxes 1-3
 * retirement, including direct/indirect DATA reads with
 * accumulator stores, T-register shift consumption and
 * preservation, C-bit write/preserve rules, and SXM/OVM
 * handling.
 * Does not own: catalog/decode or pure arithmetic effects,
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
init_add_smem(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
              uint64_t acc_a, uint64_t acc_b, uint16_t t_value,
              ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, true, status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    cpu->t_register.raw_value = t_value;
    cpu->t_register.known = true;
    /* Fresh-CPU SXM/OVM bits are unknown; the oracle needs SXM=1. */
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_SXM_MASK);
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_SXM_MASK | TI_C54X_ST1_OVM_MASK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           acc_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           acc_b) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void
test_syntax1_direct_retires(void)
{
    /* ADD dma,A with A=00 1234 0000, mem=0x5678, SXM=1:
     * A=00 1234 5678 with C=0. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x0034),
        .read_value_word = UINT16_C(0x5678),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_add_smem(&fixture, storage, UINT64_C(0x0012340000),
                      UINT64_C(0), UINT16_C(0x0400), &status);
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
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == UINT16_C(0x0034));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0x12345678));
    TI_C54X_TEST_CHECK(state.c == UINT8_C(0));
    TI_C54X_TEST_CHECK(state.c_known == true);
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x0400));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_syntax1_indirect_retires(void)
{
    /* ADD *AR5+,A with the same oracle state: A=00 1234 5678 with
     * AR5 post-incremented. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x0095),
        .read_value_word = UINT16_C(0x5678),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_add_smem(&fixture, storage, UINT64_C(0x0012340000),
                      UINT64_C(0), UINT16_C(0x0400), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(5), UINT16_C(0x0100));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == UINT16_C(0x0100));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[5] == UINT16_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0x12345678));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_syntax2_ts_retires(void)
{
    /* ADD dma,A<<TS with A=00 0000 0100, T=0x0004, mem=0x0003:
     * A=00 0000 0130. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x0434),
        .read_value_word = UINT16_C(0x0003),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_add_smem(&fixture, storage, UINT64_C(0x100),
                      UINT64_C(0), UINT16_C(0x0004), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x130));
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_syntax3_sh16_retires(void)
{
    /* ADD dma,A<<16 with A=00 0000 0100, mem=0x0002, C preset 1:
     * A=00 0002 0100 with C preserved at 1. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x3C34),
        .read_value_word = UINT16_C(0x0002),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_add_smem(&fixture, storage, UINT64_C(0x100),
                      UINT64_C(0), UINT16_C(0x0400), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    cpu->arch_state.status.st0 =
        (uint16_t)(cpu->arch_state.status.st0 | TI_C54X_ST0_C_MASK);
    cpu->arch_state.status.st0_known_mask =
        (uint16_t)(cpu->arch_state.status.st0_known_mask |
                   TI_C54X_ST0_C_MASK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x20100));
    TI_C54X_TEST_CHECK(state.c == UINT8_C(1));
    TI_C54X_TEST_CHECK(state.c_known == true);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_syntax3_carry_sets_c(void)
{
    /* ADD dma,A<<16 with A=00 FFFF FFFF, mem=0x0001, C preset 0:
     * A=01 0000 FFFF with C set to 1. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x3C34),
        .read_value_word = UINT16_C(0x0001),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_add_smem(&fixture, storage, UINT64_C(0x00FFFFFFFF),
                      UINT64_C(0), UINT16_C(0x0400), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    cpu->arch_state.status.st0 =
        (uint16_t)(cpu->arch_state.status.st0 & (uint16_t)~TI_C54X_ST0_C_MASK);
    cpu->arch_state.status.st0_known_mask =
        (uint16_t)(cpu->arch_state.status.st0_known_mask |
                   TI_C54X_ST0_C_MASK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0x010000FFFF));
    TI_C54X_TEST_CHECK(state.c == UINT8_C(1));
    TI_C54X_TEST_CHECK(state.c_known == true);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_syntax1_direct_retires();
    test_syntax1_indirect_retires();
    test_syntax2_ts_retires();
    test_syntax3_sh16_retires();
    test_syntax3_carry_sets_c();
    return 0;
}
