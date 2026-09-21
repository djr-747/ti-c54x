/* Module contract
 * Owns: public-CPU proofs for `MVDD Xmem,Ymem` retirement (TI
 * SPRU172C section MVDD, p. 4-107): the TI worked-example word
 * copies the Xmem word to Ymem through an X READ then a Y
 * WRITE, both ARs post-update, and one instruction retires
 * with a one-word PC advance.
 * Does not own: catalog/decode or pure memory effects,
 * address-planner internals, endpoint reducer behavior,
 * production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/arch_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* TI p. 4-107 example: MVDD *AR3+, *AR5+ (MOD2/MOD2, raw
 * selectors 1/3) encodes 0xE59B. */
#define MVDD_TEST_WORD_TI_EXAMPLE UINT16_C(0xE59B)
/* MOD0/MOD0 with AR2/AR2: 0xE500, alias reads twice. */
#define MVDD_TEST_WORD_MOD0_ALIAS UINT16_C(0xE500)

static void
test_ti_example_retires_with_copy(void)
{
    /* AR3 0x1000 holds 0xABCD; the word copies to (AR5)
     * 0x2000 and both ARs post-increment. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVDD_TEST_WORD_TI_EXAMPLE,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0xABCD),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;
    uint16_t ar3 = UINT16_C(0);
    uint16_t ar5 = UINT16_C(0);

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(3), UINT16_C(0x1000));
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(5), UINT16_C(0x2000));
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
                           UINT64_C(0x1000));
    TI_C54X_TEST_CHECK(fixture.requests[1].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].address_word,
                           UINT64_C(0x2000));
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].write_value_word,
                           UINT64_C(0xABCD));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                 UINT8_C(3),
                                                 &ar3) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ar3 == UINT16_C(0x1001));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                 UINT8_C(5),
                                                 &ar5) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ar5 == UINT16_C(0x2001));
}

static void
test_mod0_alias_copies_without_update(void)
{
    /* MOD0 *AR2 reads (AR2) twice and writes the word back to
     * the same address; AR2 is unchanged. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = MVDD_TEST_WORD_MOD0_ALIAS,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0x1234),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;
    uint16_t ar2 = UINT16_C(0);

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(2), UINT16_C(0x0300));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                           UINT64_C(0x0300));
    TI_C54X_TEST_CHECK(fixture.requests[1].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].address_word,
                           UINT64_C(0x0300));
    TI_C54X_TEST_CHECK_U64(fixture.requests[1].write_value_word,
                           UINT64_C(0x1234));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                 UINT8_C(2),
                                                 &ar2) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ar2 == UINT16_C(0x0300));
}

int
main(void)
{
    test_ti_example_retires_with_copy();
    test_mod0_alias_copies_without_update();
    return 0;
}
