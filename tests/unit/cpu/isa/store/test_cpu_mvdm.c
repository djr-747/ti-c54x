/* Module contract
 * Owns: public-CPU proofs for `MVDM dma,MMRx` retirement in the
 * two-word absolute flavor, including the DATA Smem read with
 * the local MMR store sidecar and rejection of unroutable MMR
 * destinations.
 * Does not own: catalog/decode or pure memory effects,
 * address-planner internals, endpoint reducer behavior,
 * production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/arch_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Absolute MVDM into AR4 (MMR 0x14): word 0 pins 0x7200 with the
 * 5-bit MMR destination at bits 4:0, word 1 is the absolute
 * 16-bit DATA address. Mirrors the 5110 boot halt word. */
#define MVDM_TEST_WORD0_AR4 UINT16_C(0x7214)
#define MVDM_TEST_WORD1 UINT16_C(0x0020)
/* MMR 5 (AH) is unroutable, so the shape gate must reject it. */
#define MVDM_TEST_WORD0_UNROUTABLE UINT16_C(0x7205)

static uint16_t
read_ar4(const ti_c54x_cpu *cpu)
{
    uint16_t value = UINT16_C(0);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                 UINT8_C(4),
                                                 &value) ==
                       TI_C54X_ARCH_STATE_OK);
    return value;
}

static void
test_absolute_retires_with_mmr_store(void)
{
    /* Fixture READY: the dma DATA read returns 0xABCD, which the
     * local MMR store sidecar publishes to AR4. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVDM_TEST_WORD0_AR4,
        .program_extension_word = MVDM_TEST_WORD1,
        .program_has_extension = true,
        .read_value_word = UINT16_C(0xABCD),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    /* Absolute dma must ignore address-register state: poison AR0
     * so a wrong indirect-path resolution would miss 0x0020. */
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(0), UINT16_C(0xFFFF));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    /* Exactly one endpoint request: the DATA read. The MMR store
     * rides the local router, not the bus. */
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                           UINT64_C(0x0020));
    TI_C54X_TEST_CHECK(read_ar4(cpu) == UINT16_C(0xABCD));
}

static void
test_unroutable_mmr_traps(void)
{
    /* Same dma but MMR 5 is unroutable: no instruction retires. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVDM_TEST_WORD0_UNROUTABLE,
        .program_extension_word = MVDM_TEST_WORD1,
        .program_has_extension = true,
        .read_value_word = UINT16_C(0xABCD),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
}

int
main(void)
{
    test_absolute_retires_with_mmr_store();
    test_unroutable_mmr_traps();
    return 0;
}
