/* Module contract
 * Owns: public-CPU RED proofs for `MPYU Smem` retirement, including
 * the TI Example 1 vector with a preset (never loaded) T and the
 * unsigned accumulator store.
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
init_mpyu_smem(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
               uint64_t acc_a, uint64_t acc_b, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, true, status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    cpu->t_register.raw_value = UINT16_C(0x4000);
    cpu->t_register.known = true;
    /* Fresh-CPU FRCT/OVM bits are unknown; the oracle needs FRCT=0. */
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 & (uint16_t)~TI_C54X_ST1_FRCT_MASK);
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
test_mpyu_direct_example1_retires(void)
{
    /* TI MPYU Example 1 direct shape: MPYU dma,A with T=0x4000,
     * mem=0xFE00, FRCT=0: A=00 3F80 0000 with T unchanged.
     * dma7 0x34 stays clear of the 0x00-0x1F MMR window so the
     * read reaches the bus. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x2434),
        .read_value_word = UINT16_C(0xFE00),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_mpyu_smem(&fixture, storage, UINT64_C(0xFF80000000),
                       UINT64_C(0), &status);
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
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x3F800000));
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x4000));
    TI_C54X_TEST_CHECK(cpu->t_register.known == true);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_mpyu_indirect_example1_retires(void)
{
    /* TI MPYU Example 1 indirect shape: MPYU *AR3,A with T=0x4000,
     * mem=0xFE00, FRCT=0: A=00 3F80 0000 with T unchanged. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = UINT16_C(0x2483),
        .read_value_word = UINT16_C(0xFE00),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_mpyu_smem(&fixture, storage, UINT64_C(0xFF80000000),
                       UINT64_C(0), &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    (void)status;
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(3), UINT16_C(0x1000));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == UINT16_C(0x1000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x3F800000));
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x4000));
    TI_C54X_TEST_CHECK(cpu->t_register.known == true);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_mpyu_direct_example1_retires();
    test_mpyu_indirect_example1_retires();
    return 0;
}
