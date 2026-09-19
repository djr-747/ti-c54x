/* Module contract
 * Owns: public-CPU proofs for `MVMM MMRx,MMRy` retirement in the
 * one-word flavor, including the MMR-routed read with the local
 * MMR store sidecar, the TI example move, and rejection of the
 * illegal nibble codes.
 * Does not own: catalog/decode or pure memory effects,
 * address-planner internals, endpoint reducer behavior,
 * production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/arch_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* MVMM AR4,AR1: word 0 pins 0xE700 with the 4-bit MMR source at
 * bits 7:4 and the 4-bit MMR destination at bits 3:0. */
#define MVMM_TEST_WORD0_AR4_AR1 UINT16_C(0xE741)
/* TI SPRU172C p. 4-116 example: MVMM SP,AR1. */
#define MVMM_TEST_WORD0_SP_AR1 UINT16_C(0xE781)
/* MMRX 9 (BK) is illegal: the leaf must reject it. */
#define MVMM_TEST_WORD0_ILLEGAL UINT16_C(0xE791)

static uint16_t
read_ar(const ti_c54x_cpu *cpu, uint8_t arf)
{
    uint16_t value = UINT16_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                 arf, &value) ==
                       TI_C54X_ARCH_STATE_OK);
    return value;
}

static void
test_ar_to_ar_retires_without_endpoint_traffic(void)
{
    /* AR4 holds 0x1234: both legs ride the local MMR router, so
     * no endpoint request issues. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVMM_TEST_WORD0_AR4_AR1,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0x0000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(4), UINT16_C(0x1234));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(read_ar(cpu, UINT8_C(1)) == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(read_ar(cpu, UINT8_C(4)) == UINT16_C(0x1234));
}

static void
test_ti_example_sp_to_ar1(void)
{
    /* MVMM SP,AR1 with SP holding 0x0200 publishes 0x0200 to
     * AR1, matching the SPRU172C example outcome. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVMM_TEST_WORD0_SP_AR1,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0x0000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x0200)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(read_ar(cpu, UINT8_C(1)) == UINT16_C(0x0200));
}

static void
test_illegal_nibble_traps(void)
{
    /* MMRX 9 decodes to MVMM but the leaf rejects it: nothing
     * retires and no endpoint traffic issues. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVMM_TEST_WORD0_ILLEGAL,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0x0000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
}

int
main(void)
{
    test_ar_to_ar_retires_without_endpoint_traffic();
    test_ti_example_sp_to_ar1();
    test_illegal_nibble_traps();
    return 0;
}
