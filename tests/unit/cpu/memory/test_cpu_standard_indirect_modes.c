/* Module contract
 * Owns: public-CPU executable proofs for representative standard-indirect WORD16
 * instructions after the neutral MOD-bearing catalog rows are installed.
 * It proves LD Smem,dst MOD0/MOD1/MOD2 address-state behavior, MOD3's
 * pre-bus invalid-operand stop, and ST T,Smem MOD3 effective-address and
 * atomic WAIT/READY publication. Does not own: catalog generation, opcode
 * handlers, pure effects, address planning, generic root transactions,
 * endpoint reduction, production code, timing, or diagnostics.
 */

#include "../isa/fixture.h"

#include "../../../../src/state/temporary/t_register.h"

#include <stddef.h>
#include <stdint.h>

static uint16_t
ld_smem_word(uint8_t destination, uint8_t mode, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x1080) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      ((uint16_t)(mode & UINT8_C(3)) << 3U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static uint16_t
st_t_smem_word(uint8_t mode, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x8C80) |
                      ((uint16_t)(mode & UINT8_C(3)) << 3U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_cpu *
init_cpu(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
         uint8_t arf, uint16_t address_word, ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, true, status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, arf, address_word);
    ti_c54x_cpu_isa_seed_sxm(cpu, UINT8_C(0));
    return cpu;
}

static void
check_load_request(const ti_c54x_cpu_isa_memory_fixture *fixture,
                   uint16_t expected_address)
{
    TI_C54X_TEST_CHECK(fixture->endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture->begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture->ready_calls == 1U);
    TI_C54X_TEST_CHECK(fixture->requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word == expected_address);
}

static void
test_ld_smem_mod0_mod1_mod2_execute(void)
{
    static const uint8_t modes[] = {UINT8_C(0), UINT8_C(1), UINT8_C(2)};
    static const uint16_t expected_ar_delta[] = {
        UINT16_C(0), UINT16_MAX, UINT16_C(1)};
    const uint8_t arf = UINT8_C(3);
    const uint16_t old_ar = UINT16_C(0x2100);

    for (size_t index = 0U; index < sizeof(modes) / sizeof(modes[0]);
         ++index) {
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ld_smem_word(UINT8_C(0), modes[index], arf),
            .read_value_word = UINT16_C(0x1234),
            .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage, arf, old_ar, &status);
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
        TI_C54X_TEST_CHECK(result.opcode_word == fixture.program_word);
        TI_C54X_TEST_CHECK(result.opcode_valid);
        TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
        check_load_request(&fixture, old_ar);
        TI_C54X_TEST_CHECK(
            cpu->arch_state.ar[arf] ==
            (uint16_t)(old_ar + expected_ar_delta[index]));
        TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 == UINT64_C(0x1234));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_ld_smem_mod3_rejects_before_data_issue(void)
{
    const uint8_t arf = UINT8_C(4);
    const uint16_t old_ar = UINT16_C(0x3200);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = ld_smem_word(UINT8_C(1), UINT8_C(3), arf),
        .read_value_word = UINT16_C(0xBEEF),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, arf, old_ar, &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    /* The public CPU maps semantic INVALID_OPERAND to INVALID_STATE. */
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(result.opcode_word == fixture.program_word);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 == UINT64_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_st_t_smem_mod3_wait_preserves_then_publishes(void)
{
    const uint8_t arf = UINT8_C(2);
    const uint16_t old_ar = UINT16_C(0x4300);
    const uint16_t t_value = UINT16_C(0xCAFE);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = st_t_smem_word(UINT8_C(3), arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, arf, old_ar, &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_t_register_write(&cpu->t_register, t_value) ==
                       TI_C54X_T_REGISTER_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == old_ar + 1U);
    TI_C54X_TEST_CHECK(fixture.requests[0].write_value_word == t_value);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[0] == fixture.ids[1]);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar + 1U);
    TI_C54X_TEST_CHECK(fixture.requests[1].write_value_word == t_value);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + 1U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_ld_smem_mod0_mod1_mod2_execute();
    test_ld_smem_mod3_rejects_before_data_issue();
    test_st_t_smem_mod3_wait_preserves_then_publishes();
    return 0;
}
