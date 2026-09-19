/* Module contract
 * Owns: public-CPU proofs for `MVMD MMRx,dma` retirement in the
 * two-word absolute flavor, including the MMR snapshot feeding
 * the DATA Smem write and rejection of unroutable MMR sources.
 * Does not own: catalog/decode or pure memory effects,
 * address-planner internals, endpoint reducer behavior,
 * production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Absolute MVMD from AR4 (MMR 0x14): word 0 pins 0x7300 with the
 * 5-bit MMR source at bits 4:0, word 1 is the absolute 16-bit
 * DATA address. */
#define MVMD_TEST_WORD0_AR4 UINT16_C(0x7314)
#define MVMD_TEST_WORD1 UINT16_C(0x0020)
/* MMR 5 (AH) is unroutable, so the shape gate must reject it. */
#define MVMD_TEST_WORD0_UNROUTABLE UINT16_C(0x7305)

static void
test_absolute_retires_with_mmr_load(void)
{
    /* AR4 holds 0x1234: the resume snapshot must publish it to
     * DATA 0x0020 through one endpoint WRITE. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVMD_TEST_WORD0_AR4,
        .program_extension_word = MVMD_TEST_WORD1,
        .program_has_extension = true,
        .read_value_word = UINT16_C(0x0000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    /* Absolute addressing ignores AR state; seeding AR4 here
     * models the live register file the MMR snapshot reads. */
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(4), UINT16_C(0x1234));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    /* Exactly one endpoint request: the DATA write carrying the
     * AR4 snapshot. */
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                           UINT64_C(0x0020));
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].write_value_word,
                           UINT64_C(0x1234));
}

static void
test_unroutable_mmr_traps(void)
{
    /* Same dma but MMR 5 is unroutable: no instruction retires
     * and no endpoint traffic issues. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVMD_TEST_WORD0_UNROUTABLE,
        .program_extension_word = MVMD_TEST_WORD1,
        .program_has_extension = true,
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
    test_absolute_retires_with_mmr_load();
    test_unroutable_mmr_traps();
    return 0;
}
