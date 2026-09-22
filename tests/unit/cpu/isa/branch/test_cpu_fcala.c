/* Module contract
 * Owns: public-CPU RED proofs for the one-word `FCALA src`
 * far accumulator call, including the TI worked-example vector
 * with the double-push TOS capture (return address then live
 * XPC), accumulator select with page derivation, WAIT resume
 * with atomic commit, DATA fault and unknown-SP/XPC
 * transactionality, and full architectural preservation outside
 * SP/PC/XPC/RTN.
 * Does not own: production CPU behavior, catalog/decode or pure
 * call effects, address planning, shared fixtures, timing,
 * diagnostics, or test registration outside the branch leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/program_counter_extension.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"
#include "../../../../support/test_arch_state_compare.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct fcala_fixture {
    uint16_t word0;
    unsigned program_calls;
    ti_c54x_program_word_address address;
    bool data_wait_then_ready;
    bool data_fault;
    bool data_waited;
    ti_c54x_bus_word16_request data_requests[2];
    unsigned data_calls;
} fcala_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    fcala_fixture *fixture = (fcala_fixture *)context;

    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(fixture->program_calls == 0U);
    fixture->address = address;
    *value = fixture->word0;
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    fcala_fixture *fixture = (fcala_fixture *)context;

    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    /* Record BEGIN-phase issues only: a RESUME re-poll carries the
     * already-recorded leg, while the two far legs issue exactly
     * once each (first TOS return address, second TOS frozen
     * XPC). */
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        TI_C54X_TEST_CHECK(fixture->data_calls < 2U);
        fixture->data_requests[fixture->data_calls] = *request;
        fixture->data_calls++;
    }
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if (fixture->data_fault) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
    if (fixture->data_wait_then_ready &&
        phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
        !fixture->data_waited) {
        fixture->data_waited = true;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        return;
    }
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
}

static ti_c54x_bus_word16_cancel_result
cancel_data(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *
init_cpu(fcala_fixture *fixture, unsigned char *storage,
         uint32_t reset_pc)
{
    const ti_c54x_bus bus = {
        .context = fixture,
        .program_read_word = read_program,
        .data_endpoint = {.context = fixture,
                          .advance = advance_data,
                          .cancel = cancel_data}};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* FORM-T2-13-04: 1111_010S_1110_0111 with Z=0; S at bit 8 selects
 * accumulator A (0) or B (1). */
static const uint16_t FCALA_WORD0_BASE = UINT16_C(0xF4E7);

static void
seed_cpu(ti_c54x_cpu *cpu, uint16_t sp_value, uint16_t xpc_value,
         uint64_t acc_a, uint64_t acc_b)
{
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_xpc(&cpu->arch_state,
                                                   xpc_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           acc_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           acc_b) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
}

static void
check_write_request(const ti_c54x_bus_word16_request *request,
                    uint16_t address, uint16_t value)
{
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(request->address_word == address);
    TI_C54X_TEST_CHECK(request->write_value_word == value);
}

static void
check_ready_case(uint32_t start_pc, uint16_t sp_before, uint8_t select,
                 uint64_t acc_a, uint64_t acc_b, uint16_t xpc_before,
                 uint16_t target, uint16_t page)
{
    fcala_fixture fixture = {
        .word0 = (uint16_t)(FCALA_WORD0_BASE |
                            (uint16_t)((uint16_t)select << 8U)),
        .program_calls = 0U,
        .data_wait_then_ready = false,
        .data_fault = false,
        .data_waited = false,
        .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, start_pc);
    ti_c54x_run_result result;
    const uint16_t expected_second_tos =
        (uint16_t)(sp_before - UINT16_C(2));
    const uint16_t expected_return =
        (uint16_t)(start_pc + UINT32_C(1));

    seed_cpu(cpu, sp_before, xpc_before, acc_a, acc_b);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;
        const ti_c54x_t_register_state before_t = cpu->t_register;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.opcode_word == fixture.word0);
        TI_C54X_TEST_CHECK(result.opcode_valid);
        TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
        TI_C54X_TEST_CHECK(fixture.address == start_pc);
        /* The first TOS WRITE carries PC+1 at old SP - 1; the
         * second carries the frozen live XPC at old SP - 2. */
        TI_C54X_TEST_CHECK(fixture.data_calls == 2U);
        check_write_request(&fixture.data_requests[0],
                            (uint16_t)(sp_before - UINT16_C(1)),
                            expected_return);
        check_write_request(&fixture.data_requests[1],
                            expected_second_tos, xpc_before);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)target);
        TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_second_tos);
        {
            uint16_t xpc = UINT16_C(0xFFFF);

            TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(
                                   &cpu->arch_state, &xpc) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK(xpc == page);
        }
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                  sizeof(before_t)) == 0);

        /* Only SP, the program counter, XPC, and the staged RTN
         * may move. */
        {
            ti_c54x_arch_state expect_arch = before_arch;

            expect_arch.pc_word = (uint32_t)target;
            expect_arch.sp = expected_second_tos;
            expect_arch.xpc = page;
            expect_arch.rtn = expected_return;
            expect_arch.scalar_known_mask = (uint8_t)(
                expect_arch.scalar_known_mask |
                (uint8_t)TI_C54X_ARCH_STATE_KNOWN_RTN);
            TI_C54X_TEST_CHECK(ti_c54x_test_arch_state_equal(&cpu->arch_state, &expect_arch));
        }
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_worked_example(void)
{
    /* TI SPRU172C FCALA A example with A 00 007F 3000: PC 0025
     * and SP 1111 become TOS[1110] 0026 and TOS[110F] 0000 with
     * SP 110F, PC 3000, and XPC 7F. */
    check_ready_case(UINT32_C(0x0025), UINT16_C(0x1111), UINT8_C(0),
                     UINT64_C(0x007F3000), UINT64_C(0x0000AABBCC),
                     UINT16_C(0x0000), UINT16_C(0x3000),
                     UINT16_C(0x007F));
}

static void
test_select_b(void)
{
    /* S=1 selects B: PC and XPC follow B bits 15-0/22-16. */
    check_ready_case(UINT32_C(0x0100), UINT16_C(0x0021), UINT8_C(1),
                     UINT64_C(0x1111222233), UINT64_C(0x00202000),
                     UINT16_C(0x0003), UINT16_C(0x2000),
                     UINT16_C(0x0020));
}

static void
test_origins_and_targets(void)
{
    /* SP wrap: old SP 0000 publishes FFFE across the two pushes. */
    check_ready_case(UINT32_C(0x0000), UINT16_C(0x0000), UINT8_C(0),
                     UINT64_C(0x00000000), UINT64_C(0x0000AABBCC),
                     UINT16_C(0x0000), UINT16_C(0x0000),
                     UINT16_C(0x0000));
}

static void
test_mmr_tos_rejects(void)
{
    /* Old SP 0001 puts the first TOS at 0000 (IMR): an
     * MMR-routed first TOS has no endpoint first WRITE and
     * MMR-1 write semantics are unevidenced. */
    fcala_fixture fixture = {.word0 = FCALA_WORD0_BASE,
                             .program_calls = 0U,
                             .data_wait_then_ready = false,
                             .data_fault = false,
                             .data_waited = false,
                             .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x0001), UINT16_C(0x0000),
             UINT64_C(0x007F3000), UINT64_C(0x0000AABBCC));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_wait_resumes_with_atomic_commit(void)
{
    fcala_fixture fixture = {
        .word0 = FCALA_WORD0_BASE,
        .program_calls = 0U,
        .data_wait_then_ready = true,
        .data_fault = false,
        .data_waited = false,
        .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x0000),
             UINT64_C(0x007F3000), UINT64_C(0x0000AABBCC));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x1111));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.data_calls == 2U);
    check_write_request(&fixture.data_requests[0], UINT16_C(0x1110),
                        UINT16_C(0x0026));
    check_write_request(&fixture.data_requests[1], UINT16_C(0x110F),
                        UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x3000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_data_fault_is_transactional(void)
{
    fcala_fixture fixture = {.word0 = FCALA_WORD0_BASE,
                             .program_calls = 0U,
                             .data_wait_then_ready = false,
                             .data_fault = true,
                             .data_waited = false,
                             .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x0000),
             UINT64_C(0x007F3000), UINT64_C(0x0000AABBCC));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_unknown_sp_rejects(void)
{
    fcala_fixture fixture = {.word0 = FCALA_WORD0_BASE,
                             .program_calls = 0U,
                             .data_wait_then_ready = false,
                             .data_fault = false,
                             .data_waited = false,
                             .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    ti_c54x_run_result result;

    /* Seed everything except SP: reset leaves SP unknown. */
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_xpc(&cpu->arch_state,
                                                   UINT16_C(0x0000)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x007F3000)) == TI_C54X_ACCUMULATOR_OK);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_unknown_xpc_rejects(void)
{
    fcala_fixture fixture = {.word0 = FCALA_WORD0_BASE,
                             .program_calls = 0U,
                             .data_wait_then_ready = false,
                             .data_fault = false,
                             .data_waited = false,
                             .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x0000),
             UINT64_C(0x007F3000), UINT64_C(0x0000AABBCC));
    cpu->arch_state.scalar_known_mask = (uint8_t)(
        cpu->arch_state.scalar_known_mask &
        (uint8_t)~(uint8_t)TI_C54X_ARCH_STATE_KNOWN_XPC);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

int main(void)
{
    test_worked_example();
    test_select_b();
    test_origins_and_targets();
    test_mmr_tos_rejects();
    test_wait_resumes_with_atomic_commit();
    test_data_fault_is_transactional();
    test_unknown_sp_rejects();
    test_unknown_xpc_rejects();
    puts("cpu_fcala: ok");
    return 0;
}
