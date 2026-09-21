/* Module contract
 *
 * Owns: public-CPU RED proofs for `ST src,Ymem || ADD/SUB
 * Xmem,dst` retirement through the store-compute root, including
 * the TI §ST||ADD and §ST||SUB example vectors, X-READ-before-
 * Y-WRITE bus order with the frozen STH value on the write leg,
 * MOD2/MOD0 post-access commits, and accumulator commit with
 * src preserved.
 * Does not own: catalog/decode or pure parallel effects,
 * address-planner internals, endpoint reducer behavior,
 * production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/block_size_register.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* TI §ST||ADD example word 0xC1C1: S=0 (src A), D=1 (dst B),
 * XMOD3 XAR=3 (*AR5+0%), YMOD0 YAR=1 (*AR3). TI §ST||SUB
 * example word 0xC5F5: same S/D/X, YMOD1 YAR=1 (*AR3-). */
#define ST_PARALLEL_ADD_WORD UINT16_C(0xC1F1)
#define ST_PARALLEL_SUB_WORD UINT16_C(0xC5F5)

static void
seed_state(ti_c54x_cpu *cpu, uint64_t a_value, uint64_t b_value,
           uint16_t y_address)
{
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(5), UINT16_C(0x0300));
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(3), y_address);
    /* MOD3 sides step by AR0 (BK=0 linear): the TI examples run
     * AR0=2, so *AR5+0% publishes 0300+2. */
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(0), UINT16_C(0x0002));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_bk(&cpu->arch_state,
                                                   UINT16_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           a_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           b_value) == TI_C54X_ACCUMULATOR_OK);
    /* Fresh-CPU SXM/OVM/ASM bits are unknown; the TI examples
     * need SXM=1, OVM=0, ASM=1. */
    cpu->arch_state.status.st1 =
        (uint16_t)((cpu->arch_state.status.st1 &
                    (uint16_t)~TI_C54X_ST1_SXM_MASK) |
                   TI_C54X_ST1_SXM_MASK);
    cpu->arch_state.status.st1 =
        (uint16_t)((cpu->arch_state.status.st1 &
                    (uint16_t)~TI_C54X_ST1_ASM_MASK) |
                   UINT16_C(0x0001));
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_SXM_MASK | TI_C54X_ST1_OVM_MASK |
                   TI_C54X_ST1_ASM_MASK);
}

static void
run_case(uint16_t opcode, uint64_t a_before, uint64_t b_before,
         uint16_t y_address, uint64_t expected_a, uint64_t expected_b,
         uint16_t expected_ar3)
{
    /* Xmem (AR5) 0x0300 holds 0x8001; Ymem (AR3) is y_address. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = opcode,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0x8001),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    seed_state(cpu, a_before, b_before, y_address);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    /* X READ first, then the Y WRITE with the frozen STH value. */
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                           UINT64_C(0x0300));
    TI_C54X_TEST_CHECK(fixture.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].address_word,
                           (uint64_t)y_address);
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].write_value_word,
                           UINT64_C(0x0842));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == expected_a);
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == expected_b);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[5] == UINT16_C(0x0302));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[3] == expected_ar3);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_add_example_retires(void)
{
    /* §ST||ADD example: B pins FF 0422 1000, A keeps the
     * pre-state FF 8421 1000, AR5 steps 0300+AR0=0302, AR3
     * holds 0200. */
    run_case(ST_PARALLEL_ADD_WORD, UINT64_C(0xFF84211000),
             UINT64_C(0x1111), UINT16_C(0x0200),
             UINT64_C(0xFF84211000), UINT64_C(0xFF04221000),
             UINT16_C(0x0200));
}

static void
test_sub_example_retires(void)
{
    /* §ST||SUB example: B pins FF FBE0 0000, A keeps
     * FF 8421 0000, AR5 steps 0300+AR0=0302, AR3 predecrements
     * 01FF to 01FE. */
    run_case(ST_PARALLEL_SUB_WORD, UINT64_C(0xFF84210000),
             UINT64_C(0x00100001), UINT16_C(0x01FF),
             UINT64_C(0xFF84210000), UINT64_C(0xFFFBE00000),
             UINT16_C(0x01FE));
}

int
main(void)
{
    test_add_example_retires();
    test_sub_example_retires();
    puts("cpu_st_parallel: ok");
    return 0;
}
