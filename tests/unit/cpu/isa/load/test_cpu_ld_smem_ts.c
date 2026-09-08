/* Module contract
 * Owns: public-CPU RED proofs for the evidence-backed fixed-MOD2
 * `LD Smem,TS,dst` load, including combined SXM/T inputs, old-AR reads,
 * postincrement, resumable completion, and transactional rejection.
 * Does not own: production CPU behavior, catalog/decode or pure load effects,
 * address planning, shared fixture implementation, timing, diagnostics, or
 * test registration outside the load leaf.
 */

#include "../fixture.h"

#include "../../../../../src/foundation/arithmetic40.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t
ld_ts_word(uint8_t destination, uint8_t arf)
{
    return (uint16_t)(UINT16_C(0x1490) |
                      ((uint16_t)(destination & UINT8_C(1)) << 8U) |
                      (uint16_t)(arf & UINT8_C(7)));
}

static uint64_t
expected_ts(uint16_t read_value, uint8_t sxm, uint16_t t_register)
{
    uint64_t value = (uint64_t)read_value;
    const uint8_t shift_bits = (uint8_t)(t_register & UINT16_C(0x3F));

    if (sxm != UINT8_C(0) && (read_value & UINT16_C(0x8000)) != UINT16_C(0)) {
        value = (TI_C54X_U40_MASK & ~UINT64_C(0xFFFF)) | value;
    }
    if (shift_bits <= UINT8_C(31)) {
        return ti_c54x_u40_logical_left_shift(value, shift_bits);
    }
    return ti_c54x_u40_arithmetic_right_shift(
        value, (uint64_t)(UINT8_C(64) - shift_bits));
}

static ti_c54x_cpu *
init_load(ti_c54x_cpu_isa_memory_fixture *fixture, void *storage,
          bool with_endpoint, uint8_t destination, uint8_t arf,
          uint16_t ar_value, uint8_t sxm, uint16_t t_register,
          ti_c54x_error *status)
{
    ti_c54x_cpu *cpu = ti_c54x_cpu_isa_init(storage, fixture, with_endpoint,
                                            status);

    TI_C54X_TEST_CHECK(*status == TI_C54X_ERROR_OK);
    ti_c54x_cpu_isa_seed_address(cpu, arf, ar_value);
    ti_c54x_cpu_isa_seed_sxm(cpu, sxm);
    cpu->t_register.raw_value = t_register;
    cpu->t_register.known = true;
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 & TI_C54X_ST1_CMPT_MASK) ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st0 & TI_C54X_ST0_ARP_MASK) ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x1111222233)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000AABBCC)) == TI_C54X_ACCUMULATOR_OK);
    (void)destination;
    return cpu;
}

static void
check_read_request(const ti_c54x_cpu_isa_memory_fixture *fixture,
                   uint16_t old_ar)
{
    TI_C54X_TEST_CHECK(fixture->requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture->requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture->requests[0].address_word == old_ar);
    TI_C54X_TEST_CHECK(fixture->requests[0].write_value_word == UINT16_C(0));
}

static void
test_ready_combines_sxm_and_t_and_postincrements_old_ar(void)
{
    static const uint16_t reads[] = {UINT16_C(0xFEDC), UINT16_C(0x8000)};
    static const uint16_t t_values[] = {UINT16_C(0x0000), UINT16_C(0x0008),
                                        UINT16_C(0x001F), UINT16_C(0x0030),
                                        UINT16_C(0x003F), UINT16_C(0xA530)};

    for (uint8_t destination = 0U; destination <= UINT8_C(1); ++destination) {
        for (uint8_t sxm = 0U; sxm <= UINT8_C(1); ++sxm) {
            for (uint8_t arf = 0U; arf < UINT8_C(8); ++arf) {
                for (size_t t_index = 0U;
                     t_index < sizeof(t_values) / sizeof(t_values[0]);
                     ++t_index) {
                    const uint16_t old_ar =
                        (uint16_t)(UINT16_C(0x2100) +
                                   (uint16_t)(arf * UINT8_C(4)));
                    for (size_t read_index = 0U;
                         read_index < sizeof(reads) / sizeof(reads[0]);
                         ++read_index) {
                        ti_c54x_cpu_isa_memory_fixture fixture = {
                            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
                            .program_word = ld_ts_word(destination, arf),
                            .read_value_word = reads[read_index],
                            .fault_status = TI_C54X_BUS_FAULT};
                        _Alignas(max_align_t) unsigned char storage[1024] =
                            {0};
                        ti_c54x_error status;
                        ti_c54x_cpu *cpu = init_load(
                            &fixture, storage, true, destination, arf, old_ar,
                            sxm, t_values[t_index], &status);
                        const ti_c54x_accumulator_state before_accumulators =
                            cpu->accumulators;
                        const ti_c54x_status_register_state before_status =
                            cpu->arch_state.status;
                        const ti_c54x_t_register_state before_t =
                            cpu->t_register;
                        const uint64_t expected =
                            expected_ts(reads[read_index], sxm,
                                        t_values[t_index]);
                        ti_c54x_run_result result =
                            ti_c54x_cpu_run(cpu, UINT64_C(1));

                        (void)status;
                        TI_C54X_TEST_CHECK(result.reason ==
                                           TI_C54X_STOP_BUDGET_EXHAUSTED);
                        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
                        TI_C54X_TEST_CHECK(result.instructions_attempted ==
                                           UINT64_C(1));
                        TI_C54X_TEST_CHECK(result.instructions_retired ==
                                           UINT64_C(1));
                        TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
                        TI_C54X_TEST_CHECK(result.opcode_word ==
                                           fixture.program_word);
                        TI_C54X_TEST_CHECK(result.opcode_valid);
                        TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
                        TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
                        TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
                        check_read_request(&fixture, old_ar);
                        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] ==
                                           (uint16_t)(old_ar + UINT16_C(1)));
                        if (destination == UINT8_C(0)) {
                            TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                                               expected);
                            TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                                               before_accumulators.b_raw40);
                        } else {
                            TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                                               expected);
                            TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                                               before_accumulators.a_raw40);
                        }
                        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state.status,
                                                  &before_status,
                                                  sizeof(before_status)) == 0);
                        TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                                  sizeof(before_t)) == 0);
                        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                                           TI_C54X_ERROR_OK);
                    }
                }
            }
        }
    }
}

static void
test_wait_resumes_same_transaction_without_refetch(void)
{
    const uint8_t destination = UINT8_C(1);
    const uint8_t arf = UINT8_C(3);
    const uint16_t old_ar = UINT16_C(0x3200);
    const uint16_t t_register = UINT16_C(0x0030);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_WAIT_THEN_READY,
        .program_word = ld_ts_word(destination, arf),
        .read_value_word = UINT16_C(0x8000),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_load(&fixture, storage, true, destination, arf,
                                 old_ar, UINT8_C(1), t_register, &status);
    const ti_c54x_arch_state before_arch = cpu->arch_state;
    const ti_c54x_accumulator_state before_accumulators = cpu->accumulators;
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                              sizeof(before_arch)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                              sizeof(before_accumulators)) == 0);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.resume_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ids[0] == fixture.ids[1]);
    TI_C54X_TEST_CHECK(fixture.requests[1].address_word == old_ar);
    TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == old_ar + UINT16_C(1));
    TI_C54X_TEST_CHECK(cpu->accumulators.b_raw40 ==
                       expected_ts(fixture.read_value_word, UINT8_C(1),
                                   t_register));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fault_is_transactional(void)
{
    const uint8_t destination = UINT8_C(0);
    const uint8_t arf = UINT8_C(1);
    const uint16_t old_ar = UINT16_C(0x4000);
    ti_c54x_cpu_isa_memory_fixture fixture = {
        .mode = TI_C54X_CPU_ISA_MEMORY_FAULT,
        .program_word = ld_ts_word(destination, arf),
        .fault_status = TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_error status;
    ti_c54x_cpu *cpu = init_load(&fixture, storage, true, destination, arf,
                                 old_ar, UINT8_C(1), UINT16_C(8), &status);
    const ti_c54x_arch_state before_arch = cpu->arch_state;
    const ti_c54x_accumulator_state before_accumulators = cpu->accumulators;
    const ti_c54x_t_register_state before_t = cpu->t_register;
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    (void)status;
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 0U);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                              sizeof(before_arch)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                              sizeof(before_accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t, sizeof(before_t)) ==
                       0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unsupported_t_zone_rejects_before_data_bus(void)
{
    static const uint16_t invalid_t_values[] = {
        UINT16_C(0x0020), UINT16_C(0x002F), UINT16_C(0xA520)};

    for (size_t index = 0U;
         index < sizeof(invalid_t_values) / sizeof(invalid_t_values[0]);
         ++index) {
        const uint8_t destination = UINT8_C(1);
        const uint8_t arf = UINT8_C(4);
        const uint16_t old_ar = UINT16_C(0x4200);
        ti_c54x_cpu_isa_memory_fixture fixture = {
            .mode = TI_C54X_CPU_ISA_MEMORY_READY,
            .program_word = ld_ts_word(destination, arf),
            .read_value_word = UINT16_C(0xFFFF),
            .fault_status = TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_error status;
        ti_c54x_cpu *cpu = init_load(
            &fixture, storage, true, destination, arf, old_ar, UINT8_C(1),
            invalid_t_values[index], &status);
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators = cpu->accumulators;
        const ti_c54x_t_register_state before_t = cpu->t_register;
        ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

        (void)status;
        /* Public CPU maps the semantic INVALID_OPERAND stop to INVALID_STATE. */
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == before_arch.pc_word);
        TI_C54X_TEST_CHECK(cpu->arch_state.ar[arf] == before_arch.ar[arf]);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                  sizeof(before_t)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

int
main(void)
{
    test_ready_combines_sxm_and_t_and_postincrements_old_ar();
    test_wait_resumes_same_transaction_without_refetch();
    test_fault_is_transactional();
    test_unsupported_t_zone_rejects_before_data_bus();
    return 0;
}
