/* Module contract
 * Owns: public-CPU RED proofs for `POLY Smem` retirement, including
 * the TI section example with live T and the SXM-widened B load.
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
init_poly_smem(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
               uint64_t acc_a, uint64_t acc_b, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, true, status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    cpu->t_register.raw_value = UINT16_C(0x5678);
    cpu->t_register.known = true;
    /* Fresh-CPU FRCT/OVM/SXM bits are unknown; the oracle needs
     * FRCT=0, OVM=0, SXM=0. */
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 &
                   (uint16_t)~(TI_C54X_ST1_FRCT_MASK |
                               TI_C54X_ST1_SXM_MASK));
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_FRCT_MASK | TI_C54X_ST1_OVM_MASK |
                   TI_C54X_ST1_SXM_MASK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           acc_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           acc_b) == TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void
test_poly_indirect_example_retires(void)
{
    /* TI SPRU172C POLY example shape: POLY *AR3+ with A=00 1234
     * 0000, B=00 0001 0000, T=0x5678, mem=0x2000, FRCT=0, SXM=0:
     * A=00 0627 0000, B=00 2000 0000, T untouched, AR3
     * post-incremented. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x3693),
        .read_value_word = UINT16_C(0x2000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_poly_smem(&fixture, storage, UINT64_C(0x0012340000),
                       UINT64_C(0x00010000), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(3), UINT16_C(0x0200));
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
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == UINT16_C(0x0201));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x6270000));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == UINT64_C(0x20000000));
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x5678));
    TI_C54X_TEST_CHECK(cpu->t_register.known == true);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_poly_indirect_example_retires();
    return 0;
}
