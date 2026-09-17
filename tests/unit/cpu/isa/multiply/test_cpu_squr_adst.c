/* Module contract
 * Owns: public-CPU RED proofs for `SQUR A,dst` retirement, including
 * the TI Example 2 register-only completion into B with A and T
 * preservation, the D=0 overwrite of A, and endpoint silence (no
 * DATA traffic for the register-only form).
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
init_squr_adst(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
               uint64_t acc_a, uint64_t acc_b, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, true, status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    cpu->t_register.raw_value = UINT16_C(0xBEEF);
    cpu->t_register.known = false;
    /* Fresh-CPU FRCT/OVM bits are unknown; the oracle needs FRCT=1. */
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_FRCT_MASK);
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_FRCT_MASK | TI_C54X_ST1_OVM_MASK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           acc_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           acc_b) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void
test_example2_to_b_retires(void)
{
    /* TI SQUR Example 2: SQUR A,B with A=00 000F 0000, FRCT=1:
     * B=00 0000 01C2, A and T preserved, no DATA traffic. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0xF58D),
        .read_value_word = UINT16_C(0),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_squr_adst(&fixture, storage, UINT64_C(0x00000F0000),
                       UINT64_C(0x0001010101), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0x00000F0000));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 ==
                       UINT64_C(0x00000001C2));
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0xBEEF));
    TI_C54X_TEST_CHECK(cpu->t_register.known == false);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_dst_a_overwrites(void)
{
    /* 0xF48D (D=0): the square lands in A, T stays unknown. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0xF48D),
        .read_value_word = UINT16_C(0),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_squr_adst(&fixture, storage, UINT64_C(0x00000F0000),
                       UINT64_C(0x0001010101), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 ==
                       UINT64_C(0x00000001C2));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 ==
                       UINT64_C(0x0001010101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    /* Register-only form seeds no address. */
    (void)ti_c54x_cpu_isa_seed_address;
    test_example2_to_b_retires();
    test_dst_a_overwrites();
    return 0;
}
