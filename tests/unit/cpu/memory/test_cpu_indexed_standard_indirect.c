/* Module contract
 * Owns: public-CPU executable proofs for representative indexed MOD5/MOD6
 * WORD16 reads and writes, including old-AR addressing, selected-AR commit,
 * AR0 preservation, resumable WAIT/READY publication, and pre-bus boundary
 * or AR0-alias rejection. Does not own: catalog/decode metadata, pure handler
 * effects, address-planner internals, endpoint implementation, timing, or
 * diagnostics.
 */

#include "../isa/fixture.h"

#include "../../../../src/state/temporary/t_register.h"

#include <stddef.h>
#include <stdint.h>

static uint16_t
ld_smem_indexed_word(uint8_t mode, uint8_t arf)
{
    return (uint16_t)((mode == UINT8_C(5) ? UINT16_C(0x10A8)
                                         : UINT16_C(0x10B0)) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static uint16_t
ldr_indexed_word(uint8_t mode, uint8_t arf)
{
    /* Destination-accumulator bit sits at first-word bit 8; select B so the
     * rounded write lands in a known accumulator. */
    return (uint16_t)((mode == UINT8_C(5) ? UINT16_C(0x16A8)
                                         : UINT16_C(0x16B0)) |
                      (uint16_t)(arf & UINT8_C(7)) | UINT16_C(0x0100));
}

static uint16_t
st_t_smem_indexed_word(uint8_t mode, uint8_t arf)
{
    return (uint16_t)((mode == UINT8_C(5) ? UINT16_C(0x8CA8)
                                         : UINT16_C(0x8CB0)) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static ti_c54x_cpu *
init_indexed_cpu(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
                 uint8_t arf, uint16_t ar_value, uint16_t ar0_value,
                 ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, true, status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, arf, ar_value);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                           &cpu->arch_state, UINT8_C(0), ar0_value) ==
                       TI_C54X_ARCH_STATE_OK);
    ti_c54x_cpu_isa_seed_sxm(cpu, UINT8_C(0));
    return cpu;
}

static void
check_common_result(const ti_c54x_run_result *result,
                    ti_c54x_stop_reason reason, uint64_t attempted,
                    uint64_t retired)
{
    TI_C54X_TEST_CHECK(result->reason == reason);
    TI_C54X_TEST_CHECK(result->bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result->instructions_attempted == attempted);
    TI_C54X_TEST_CHECK(result->instructions_retired == retired);
}

static void
test_load_mod5_ready_subtracts_after_old_address(void)
{
    const uint8_t arf = UINT8_C(3);
    const uint16_t old_ar = UINT16_C(0x2450);
    const uint16_t old_ar0 = UINT16_C(0x0012);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = ld_smem_indexed_word(UINT8_C(5), arf),
        .read_value_word = UINT16_C(0x8123),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_indexed_cpu(&fixture, storage, arf, old_ar, old_ar0,
                                        &status);
    const ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    check_common_result(&result, TI_C54X_STOP_BUDGET_EXHAUSTED, 1U, 1U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar - old_ar0);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[0] == old_ar0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_load_mod6_wait_then_ready_adds_after_resume(void)
{
    const uint8_t arf = UINT8_C(5);
    const uint16_t old_ar = UINT16_C(0x3450);
    const uint16_t old_ar0 = UINT16_C(0x0020);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = ld_smem_indexed_word(UINT8_C(6), arf),
        .read_value_word = UINT16_C(0x1234),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_indexed_cpu(&fixture, storage, arf, old_ar, old_ar0,
                                        &status);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    check_common_result(&result, TI_C54X_STOP_BUS_WAIT, 1U, 0U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[0] == old_ar0);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    check_common_result(&result, TI_C54X_STOP_BUDGET_EXHAUSTED, 0U, 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[0] == fixture.ids[1]);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + old_ar0);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[0] == old_ar0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_store_mod5_wait_then_ready_writes_old_address(void)
{
    const uint8_t arf = UINT8_C(2);
    const uint16_t old_ar = UINT16_C(0x4200);
    const uint16_t old_ar0 = UINT16_C(0x0021);
    const uint16_t t_value = UINT16_C(0xCAFE);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = st_t_smem_indexed_word(UINT8_C(5), arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_indexed_cpu(&fixture, storage, arf, old_ar, old_ar0,
                                        &status);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_t_register_write(&cpu->t_register, t_value) ==
                       TI_C54X_T_REGISTER_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    (void)status;
    check_common_result(&result, TI_C54X_STOP_BUS_WAIT, 1U, 0U);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == old_ar);
    TI_C54X_TEST_CHECK(fixture.requests[0].write_value_word == t_value);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[0] == old_ar0);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    check_common_result(&result, TI_C54X_STOP_BUDGET_EXHAUSTED, 0U, 1U);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(fixture.requests[1].write_value_word == t_value);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar - old_ar0);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[0] == old_ar0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_store_mod6_ready_adds_after_write(void)
{
    const uint8_t arf = UINT8_C(7);
    const uint16_t old_ar = UINT16_C(0x1200);
    const uint16_t old_ar0 = UINT16_C(0x0030);
    const uint16_t t_value = UINT16_C(0x5AA5);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = st_t_smem_indexed_word(UINT8_C(6), arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_indexed_cpu(&fixture, storage, arf, old_ar, old_ar0,
                                        &status);

    TI_C54X_TEST_CHECK(ti_c54x_t_register_write(&cpu->t_register, t_value) ==
                       TI_C54X_T_REGISTER_OK);
    (void)status;
    {
        const ti_c54x_run_result ready = ti_c54x_cpu_run(cpu, UINT64_C(1));
        check_common_result(&ready, TI_C54X_STOP_BUDGET_EXHAUSTED, 1U, 1U);
        TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
        TI_C54X_TEST_CHECK(fixture.requests[0].address_word == old_ar);
        TI_C54X_TEST_CHECK(fixture.requests[0].write_value_word == t_value);
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + old_ar0);
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[0] == old_ar0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_rejects_before_endpoint(uint16_t word, uint8_t arf, uint16_t ar_value,
                              uint16_t ar0_value)
{
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = word,
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_indexed_cpu(&fixture, storage, arf, ar_value,
                                        ar0_value, &status);
    const ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    check_common_result(&result, TI_C54X_STOP_INVALID_STATE, 1U, 0U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] ==
                       (arf == UINT8_C(0) ? ar0_value : ar_value));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[0] == ar0_value);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_indexed_alias_and_boundaries_reject_pre_bus(void)
{
    check_rejects_before_endpoint(ld_smem_indexed_word(5U, 0U), 0U,
                                  UINT16_C(0x2100), UINT16_C(0x0010));
    check_rejects_before_endpoint(ld_smem_indexed_word(5U, 4U), 4U,
                                  UINT16_C(0x0005), UINT16_C(0x0006));
    check_rejects_before_endpoint(st_t_smem_indexed_word(6U, 6U), 6U,
                                  UINT16_C(0xFFF0), UINT16_C(0x0020));
    check_rejects_before_endpoint(ldr_indexed_word(6U, 0U), 0U,
                                  UINT16_C(0x2100), UINT16_C(0x0010));
}

static void
test_ldr_mod5_ready_rounds_shifted_value(void)
{
    const uint8_t arf = UINT8_C(1);
    const uint16_t old_ar = UINT16_C(0x5100);
    const uint16_t old_ar0 = UINT16_C(0x0004);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_READY,
        .program_word = ldr_indexed_word(UINT8_C(5), arf),
        .read_value_word = UINT16_C(0x0001),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_indexed_cpu(&fixture, storage, arf, old_ar, old_ar0,
                                        &status);
    const ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    check_common_result(&result, TI_C54X_STOP_BUDGET_EXHAUSTED, 1U, 1U);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word == old_ar);
    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 == UINT64_C(0x18000));
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar - old_ar0);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[0] == old_ar0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_load_mod5_ready_subtracts_after_old_address();
    test_load_mod6_wait_then_ready_adds_after_resume();
    test_store_mod5_wait_then_ready_writes_old_address();
    test_store_mod6_ready_adds_after_write();
    test_ldr_mod5_ready_rounds_shifted_value();
    test_indexed_alias_and_boundaries_reject_pre_bus();
    return 0;
}
