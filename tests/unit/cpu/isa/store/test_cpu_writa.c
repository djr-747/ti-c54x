/* Module contract
 * Owns: public-CPU proofs for `WRITA Smem` retirement in the
 * one-word direct and indirect flavors, including the Smem
 * read feeding the accumulator-addressed program write, AR
 * update commit, and rejection of MOD3 reads,
 * extension-requiring cookies, and unknown accumulator A.
 * Does not own: catalog/decode or pure memory effects,
 * address-planner internals, endpoint reducer behavior,
 * production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/arch_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Direct dma 5 with DP 006 and A 0x0023: the word at 0x0305
 * lands in Pmem 0x23 (TI WRITA p. 4-196 example shape). */
#define WRITA_TEST_WORD0_DIRECT UINT16_C(0x7F05)
/* MOD2 cookie (mode 2, ARF 7): post-increment addresses AR7
 * and posts AR7+1. */
#define WRITA_TEST_WORD0_INDIRECT UINT16_C(0x7F97)
/* MOD3 reads are unsupported in the address planner
 * (pre-access updates on reads): the root rejects
 * transactionally with no endpoint traffic. */
#define WRITA_TEST_WORD0_MOD3_READ UINT16_C(0x7F9F)
/* mode4 4 needs an extension word: the one-word leaf rejects. */
#define WRITA_TEST_WORD0_BAD_MODE UINT16_C(0x7FA2)

static ti_c54x_cpu *
init_writa(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
           uint16_t accumulator_a_low16, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, fixture, true, status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           (uint64_t)accumulator_a_low16) ==
                       TI_C54X_ACCUMULATOR_OK);
    return cpu;
}

static void
test_direct_retires_with_copy(void)
{
    /* The 0x8A55 word at 0x0305 copies to Pmem 0x23 through
     * a DATA read plus an accumulator-addressed program write. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = WRITA_TEST_WORD0_DIRECT,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0x8A55),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_writa(&fixture, storage, UINT16_C(0x0023), &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&cpu->arch_state,
                                                  UINT16_C(0x006)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                           UINT64_C(0x0305));
    TI_C54X_TEST_CHECK(fixture.requests[1].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM);
    TI_C54X_TEST_CHECK(fixture.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].address_word,
                           UINT64_C(0x0023));
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].write_value_word,
                           UINT64_C(0x8A55));
}

static void
test_indirect_retires_with_ar_update(void)
{
    /* MOD2 reads 0x0FFE into Pmem 0x2000 and posts AR7 to
     * 0x0FFF (AR0 seeds CMPT0/ARP0 liveness only). The Smem
     * leg reads first, the PROGRAM leg writes second. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = WRITA_TEST_WORD0_INDIRECT,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0x1234),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_writa(&fixture, storage, UINT16_C(0x2000), &status);
    ti_c54x_run_result result;
    uint16_t ar7 = UINT16_C(0);

    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(0), UINT16_C(0x0002));
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(7), UINT16_C(0x0FFE));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                           UINT64_C(0x0FFE));
    TI_C54X_TEST_CHECK(fixture.requests[1].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM);
    TI_C54X_TEST_CHECK(fixture.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].address_word,
                           UINT64_C(0x2000));
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].write_value_word,
                           UINT64_C(0x1234));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                 UINT8_C(7),
                                                 &ar7) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ar7 == UINT16_C(0x0FFF));
}

static void
test_bad_mode_traps(void)
{
    /* mode4 4 needs an extension word the bundle lacks: nothing
     * retires and no endpoint traffic issues. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = WRITA_TEST_WORD0_BAD_MODE,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0x1234),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_writa(&fixture, storage, UINT16_C(0x2000), &status);
    ti_c54x_run_result result;

    (void)cpu;
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
}

static void
test_a_unknown_traps(void)
{
    /* Accumulator A unknown: the input gate rejects with no
     * endpoint traffic. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = WRITA_TEST_WORD0_DIRECT,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0x8A55),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&cpu->arch_state,
                                                  UINT16_C(0x006)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
}

static void
test_mod3_read_rejects(void)
{
    /* MOD3 Smem read: the address planner rejects the
     * pre-access update, so nothing retires and no endpoint
     * traffic issues. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = WRITA_TEST_WORD0_MOD3_READ,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0x1234),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        init_writa(&fixture, storage, UINT16_C(0x2000), &status);
    ti_c54x_run_result result;
    uint16_t ar7 = UINT16_C(0);

    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(0), UINT16_C(0x0002));
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(7), UINT16_C(0x0FFE));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                 UINT8_C(7),
                                                 &ar7) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ar7 == UINT16_C(0x0FFE));
}

int
main(void)
{
    test_direct_retires_with_copy();
    test_indirect_retires_with_ar_update();
    test_mod3_read_rejects();
    test_bad_mode_traps();
    test_a_unknown_traps();
    return 0;
}
