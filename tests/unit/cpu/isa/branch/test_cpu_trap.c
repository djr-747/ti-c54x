/* Module contract
 * Owns: public-CPU proofs for `TRAP K` retirement (TI SPRU172C
 * section TRAP, p. 4-195): the K vector loads into PC, PC+1
 * pushes to TOS through one endpoint WRITE, SP decrements,
 * and no status state changes.
 * Does not own: catalog/decode or pure memory effects,
 * address-planner internals, endpoint reducer behavior,
 * production code, timing, or diagnostics.
 */

#include "../fixture.h"

#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/status/registers.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* TI p. 4-195 example: TRAP 10h (K=16, word 0xF4D0). */
#define TRAP_TEST_WORD_TI_EXAMPLE UINT16_C(0xF4D0)

static void
test_ti_example_retires_with_vector(void)
{
    /* IPTR 0x100 (pmst 0x8000): vector is 0x8000 + (16 << 2)
     * = 0x8040. PC+1 = 0x0001 pushes to (SP) 0x03FE. INTM
     * starts set and must stay set; ST0/ST1/IFR untouched. */
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = TRAP_TEST_WORD_TI_EXAMPLE,
        .program_has_extension = false,
        .read_value_word = UINT16_C(0),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu =
        ti_c54x_cpu_isa_init(storage, &fixture, true, &status);
    ti_c54x_run_result result;
    uint16_t sp = UINT16_C(0);

    TI_C54X_TEST_CHECK(status == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x03FF)) ==
                       TI_C54X_ARCH_STATE_OK);
    ti_c54x_cpu_isa_seed_address(cpu, UINT8_C(2), UINT16_C(0x5555));
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_st0(
                           &cpu->arch_state.status,
                           UINT16_C(0x0000)) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_st1(
                           &cpu->arch_state.status,
                           UINT16_C(0x0800)) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_pmst(
                           &cpu->arch_state.status,
                           UINT16_C(0x8000)) ==
                       TI_C54X_STATUS_REGISTER_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x8040));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].address_word,
                           UINT64_C(0x03FE));
    TI_C54X_TEST_CHECK_U64(fixture.requests[0].write_value_word,
                           UINT64_C(0x0001));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_sp(&cpu->arch_state,
                                                 &sp) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(sp == UINT16_C(0x03FE));
    /* No status effect: ST0, ST1 (INTM still set), and IFR
     * preserved exactly; unlisted AR state untouched. */
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 == UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == UINT16_C(0x0800));
    TI_C54X_TEST_CHECK(cpu->arch_state.ifr == UINT16_C(0));
    {
        uint16_t ar2 = UINT16_C(0);

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                     UINT8_C(2),
                                                     &ar2) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(ar2 == UINT16_C(0x5555));
    }
}

int
main(void)
{
    test_ti_example_retires_with_vector();
    return 0;
}
