/* Module contract
 * Owns: RED public-CPU retirement proofs for the ST
 * parallel-MAC/MAS/MPY rows over the S115 store-compute seam:
 * TI worked-example retirement with X-READ-before-Y-WRITE
 * order, frozen STH values, T/FRCT preservation, and AR
 * post-access commits.
 * Does not own: opcode semantics, T/FRCT/OVM policy, or the
 * store-compute child root.
 */

#include "../fixture.h"

#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/block_size_register.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* TI §ST||MAC Example 1 word 0xD136: R=0, S=0 (src A), D=1
 * (dst B), XMOD0 XAR=3 (*AR5), YMOD1 YAR=2 (*AR4-). */
/* TI §ST||MAS Example 1 word 0xD93A: same S/D, XMOD0,
 * YMOD2 (*AR4+). */
/* TI §ST||MPY example word 0xCDB9: S=0, D=1, XMOD2 XAR=3
 * (*AR5+), YMOD2 YAR=1 (*AR3+). */
#define ST_PARALLEL_MAC_WORD UINT16_C(0xD136)
#define ST_PARALLEL_MAS_WORD UINT16_C(0xD93A)
#define ST_PARALLEL_MPY_WORD UINT16_C(0xCDB9)

static void
seed_state(ti_c54x_cpu *cpu, uint64_t a_value, uint64_t b_value,
           uint16_t t_value, uint8_t frct, uint8_t asm_value,
           uint8_t x_arf, uint16_t x_address, uint8_t y_arf,
           uint16_t y_address)
{
    ti_c54x_cpu_isa_seed_address(cpu, x_arf, x_address);
    ti_c54x_cpu_isa_seed_address(cpu, y_arf, y_address);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           a_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           b_value) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = t_value;
    cpu->t_register.known = true;
    if (frct != UINT8_C(0)) {
        cpu->arch_state.status.st1 =
            (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_FRCT_MASK);
    } else {
        cpu->arch_state.status.st1 =
            (uint16_t)(cpu->arch_state.status.st1 &
                       (uint16_t)~TI_C54X_ST1_FRCT_MASK);
    }
    cpu->arch_state.status.st1 =
        (uint16_t)((cpu->arch_state.status.st1 &
                    (uint16_t)~TI_C54X_ST1_ASM_MASK) |
                   (uint16_t)asm_value);
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_FRCT_MASK | TI_C54X_ST1_OVM_MASK |
                   TI_C54X_ST1_ASM_MASK);
}

static void
run_case(uint16_t opcode, uint64_t a_before, uint64_t b_before,
         uint16_t t_value, uint8_t frct, uint8_t asm_value,
         uint8_t x_arf, uint16_t x_address, uint16_t x_read,
         uint8_t y_arf, uint16_t y_address, uint16_t y_write,
         uint64_t expected_a, uint64_t expected_b,
         uint16_t expected_x_ar, uint16_t expected_y_ar)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = opcode,
        .program_has_extension = false,
        .read_value_word = x_read,
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;
    ti_c54x_state state;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    seed_state(cpu, a_before, b_before, t_value, frct, asm_value, x_arf,
               x_address, y_arf, y_address);
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
                           (uint64_t)x_address);
    TI_C54X_TEST_CHECK(fixture.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].address_word,
                           (uint64_t)y_address);
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].write_value_word,
                           (uint64_t)y_write);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == expected_a);
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == expected_b);
    /* No T load: T keeps the live input. */
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == t_value);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[x_arf] == expected_x_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[y_arf] == expected_y_ar);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_mac_example_retires(void)
{
    /* TI §ST||MAC Example 1: B pins 00 010C 9511, A keeps
     * 00 0011 1111, AR5 holds 0200, AR4 predecrements to 00FF. */
    run_case(ST_PARALLEL_MAC_WORD, UINT64_C(0x0000111111),
             UINT64_C(0x1111), UINT16_C(0x0400), UINT8_C(0),
             UINT8_C(5), UINT8_C(5), UINT16_C(0x0200),
             UINT16_C(0x4321), UINT8_C(4), UINT16_C(0x0100),
             UINT16_C(0x0222), UINT64_C(0x0000111111),
             UINT64_C(0x00010C9511), UINT16_C(0x0200),
             UINT16_C(0x00FF));
}

static void
test_mas_example_retires(void)
{
    /* TI §ST||MAS Example 1: B pins FF FEF3 8D11, A keeps
     * 00 0011 1111, AR5 holds 0200, AR4 post-increments to 0101. */
    run_case(ST_PARALLEL_MAS_WORD, UINT64_C(0x0000111111),
             UINT64_C(0x1111), UINT16_C(0x0400), UINT8_C(0),
             UINT8_C(5), UINT8_C(5), UINT16_C(0x0200),
             UINT16_C(0x4321), UINT8_C(4), UINT16_C(0x0100),
             UINT16_C(0x0222), UINT64_C(0x0000111111),
             UINT64_C(0xFFFEF38D11), UINT16_C(0x0200),
             UINT16_C(0x0101));
}

static void
test_mpy_example_retires(void)
{
    /* TI §ST||MPY example: B pins 00 2000 0000, A keeps
     * FF 8421 1234, AR5/AR3 post-increment to 0301/0201. */
    run_case(ST_PARALLEL_MPY_WORD, UINT64_C(0xFF84211234),
             UINT64_C(0x00), UINT16_C(0x4000), UINT8_C(1),
             UINT8_C(0), UINT8_C(5), UINT16_C(0x0300),
             UINT16_C(0x4000), UINT8_C(3), UINT16_C(0x0200),
             UINT16_C(0x8421), UINT64_C(0xFF84211234),
             UINT64_C(0x0020000000), UINT16_C(0x0301),
             UINT16_C(0x0201));
}

int
main(void)
{
    test_mac_example_retires();
    test_mas_example_retires();
    test_mpy_example_retires();
    puts("cpu_st_parallel_mac: ok");
    return 0;
}
