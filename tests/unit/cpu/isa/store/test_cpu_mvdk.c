/* Module contract
 * Owns: public-CPU proofs for `MVDK Smem,dmad` retirement in the
 * two-word direct and indirect flavors, including the Smem read
 * feeding the absolute dmad write, AR update commit, and
 * rejection of extension-requiring cookies.
 * Does not own: catalog/decode or pure memory effects,
 * address-planner internals, endpoint reducer behavior,
 * production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/arch_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* MVDK 10,8000h (TI example 1 shape): direct dma 0x0A with DP
 * 004 reads 0x020A; word 1 is the absolute dmad. */
#define MVDK_TEST_WORD0_DIRECT UINT16_C(0x710A)
#define MVDK_TEST_WORD1 UINT16_C(0x8000)
/* MVDK *AR3-,1000h (TI example 2 shape): MOD1 with ARF 3. */
#define MVDK_TEST_WORD0_INDIRECT UINT16_C(0x718B)
#define MVDK_TEST_WORD1_INDIRECT UINT16_C(0x1000)
/* mode4 4 needs an extension word: the two-word leaf rejects. */
#define MVDK_TEST_WORD0_BAD_MODE UINT16_C(0x71A2)

static void
test_direct_retires_with_copy(void)
{
    /* DP 004 selects page 0x0200: the 0xABCD word at 0x020A
     * copies to dmad 0x8000 through two endpoint requests. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVDK_TEST_WORD0_DIRECT,
        .program_extension_word = MVDK_TEST_WORD1,
        .program_has_extension = true,
        .read_value_word = UINT16_C(0xABCD),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&cpu->arch_state,
                                                  UINT16_C(0x004)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                           UINT64_C(0x020A));
    TI_C54X_TEST_CHECK(fixture.requests[1].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].address_word,
                           UINT64_C(0x8000));
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].write_value_word,
                           UINT64_C(0xABCD));
}

static void
test_indirect_retires_with_ar_update(void)
{
    /* *AR3- reads 0x01FF and post-decrements AR3; the word
     * copies to dmad 0x1000. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVDK_TEST_WORD0_INDIRECT,
        .program_extension_word = MVDK_TEST_WORD1_INDIRECT,
        .program_has_extension = true,
        .read_value_word = UINT16_C(0x1234),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;
    uint16_t ar3 = UINT16_C(0);

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(3), UINT16_C(0x01FF));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                           UINT64_C(0x01FF));
    TI_C54X_TEST_CHECK(fixture.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].address_word,
                           UINT64_C(0x1000));
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].write_value_word,
                           UINT64_C(0x1234));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                 UINT8_C(3),
                                                 &ar3) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ar3 == UINT16_C(0x01FE));
}

static void
test_bad_mode_traps(void)
{
    /* mode4 4 needs an extension word the bundle lacks: nothing
     * retires and no endpoint traffic issues. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVDK_TEST_WORD0_BAD_MODE,
        .program_extension_word = MVDK_TEST_WORD1_INDIRECT,
        .program_has_extension = true,
        .read_value_word = UINT16_C(0x1234),
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
    test_direct_retires_with_copy();
    test_indirect_retires_with_ar_update();
    test_bad_mode_traps();
    return 0;
}
