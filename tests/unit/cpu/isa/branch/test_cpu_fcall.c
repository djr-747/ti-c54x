/* Module contract
 * Owns: public-CPU RED proofs for the two-word `FCALL pmad`
 * far call, including the TI worked-example vector with the
 * double-push TOS capture (return address then live XPC),
 * WAIT resume with atomic commit, frozen-XPC second push,
 * DATA fault and unknown-SP/XPC transactionality, two-word
 * fetch ordering, and full architectural preservation outside
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
#include <string.h>

typedef struct fcall_fixture {
    uint16_t words[2];
    unsigned program_calls;
    ti_c54x_program_word_address addresses[2];
    bool fail_second;
    /* DATA endpoint script: WAIT once, then READY; or FAULT. */
    bool data_wait_then_ready;
    bool data_fault;
    bool data_waited;
    ti_c54x_bus_word16_request data_requests[2];
    unsigned data_calls;
} fcall_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    fcall_fixture *fixture = (fcall_fixture *)context;
    const unsigned index = fixture->program_calls;

    TI_C54X_TEST_CHECK(index < 2U);
    TI_C54X_TEST_CHECK(value != NULL);
    if (fixture->fail_second && index == 1U) {
        return TI_C54X_BUS_FAULT;
    }
    fixture->addresses[index] = address;
    *value = fixture->words[index];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    fcall_fixture *fixture = (fcall_fixture *)context;

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
init_cpu(fcall_fixture *fixture, unsigned char *storage,
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

/* FORM-T2-13-05: 1111_1001_1PPP_PPPP with Z=0; word1 is the copied
 * raw pmad. */
static const uint16_t FCALL_WORD0_BASE = UINT16_C(0xF980);

static void
seed_cpu(ti_c54x_cpu *cpu, uint16_t sp_value, uint16_t xpc_value)
{
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
    cpu->arch_state.status.pmst = UINT16_C(0x3C3C);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_xpc(&cpu->arch_state,
                                                   xpc_value) ==
                       TI_C54X_ARCH_STATE_OK);
    /* CMPT/ARP stay unknown: pushes are mode-independent end to end. */
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x1111222233)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000AABBCC)) == TI_C54X_ACCUMULATOR_OK);
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
check_ready_case(uint32_t start_pc, uint16_t sp_before, uint8_t page,
                 uint16_t xpc_before, uint16_t pmad)
{
    fcall_fixture fixture = {
        .words = {(uint16_t)(FCALL_WORD0_BASE | page), pmad},
        .program_calls = 0U,
        .addresses = {0U, 0U},
        .fail_second = false,
        .data_wait_then_ready = false,
        .data_fault = false,
        .data_waited = false,
        .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, start_pc);
    ti_c54x_run_result result;
    ti_c54x_state state;
    const uint16_t expected_first_tos =
        (uint16_t)(sp_before - UINT16_C(1));
    const uint16_t expected_second_tos =
        (uint16_t)(sp_before - UINT16_C(2));
    const uint16_t expected_return =
        (uint16_t)(start_pc + UINT32_C(2));

    seed_cpu(cpu, sp_before, xpc_before);
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
        TI_C54X_TEST_CHECK(result.opcode_word == fixture.words[0]);
        TI_C54X_TEST_CHECK(result.opcode_valid);
        TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
        TI_C54X_TEST_CHECK(fixture.addresses[0] == start_pc);
        TI_C54X_TEST_CHECK(fixture.addresses[1] == start_pc + UINT32_C(1));
        /* The first TOS WRITE carries PC+2 at old SP - 1; the
         * second carries the frozen live XPC at old SP - 2. */
        TI_C54X_TEST_CHECK(fixture.data_calls == 2U);
        check_write_request(&fixture.data_requests[0],
                            expected_first_tos, expected_return);
        check_write_request(&fixture.data_requests[1],
                            expected_second_tos, xpc_before);
        TI_C54X_TEST_CHECK(result.pc_word == (uint32_t)pmad);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)pmad);
        TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_second_tos);
        {
            uint16_t xpc = UINT16_C(0xFFFF);

            TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(
                                   &cpu->arch_state, &xpc) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK(xpc == (uint16_t)page);
        }
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                  sizeof(before_t)) == 0);

        /* Only SP, the program counter, XPC, and the staged RTN
         * may move. */
        {
            ti_c54x_arch_state expect_arch = before_arch;

            expect_arch.pc_word = (uint32_t)pmad;
            expect_arch.sp = expected_second_tos;
            expect_arch.xpc = (uint16_t)page;
            expect_arch.rtn = expected_return;
            expect_arch.scalar_known_mask = (uint8_t)(
                expect_arch.scalar_known_mask |
                (uint8_t)TI_C54X_ARCH_STATE_KNOWN_RTN);
            TI_C54X_TEST_CHECK(ti_c54x_test_arch_state_equal(&cpu->arch_state, &expect_arch));
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == (uint32_t)pmad);
        TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_worked_example(void)
{
    /* TI SPRU172C FCALL 013333h example: PC 0025 and SP 1111
     * become TOS[1110] 0027 and TOS[110F] 0000 with SP 110F,
     * PC 3333, and XPC 01. */
    check_ready_case(UINT32_C(0x0025), UINT16_C(0x1111), UINT8_C(0x01),
                     UINT16_C(0x0000), UINT16_C(0x3333));
}

static void
test_origins_and_targets(void)
{
    check_ready_case(UINT32_C(0x0000), UINT16_C(0x0021), UINT8_C(0x00),
                     UINT16_C(0x0000), UINT16_C(0x0000));
    check_ready_case(UINT32_C(0x1F45), UINT16_C(0x8000), UINT8_C(0x7F),
                     UINT16_C(0x0012), UINT16_C(0x2000));
    /* SP wrap: old SP 0000 publishes FFFE across the two pushes. */
    check_ready_case(UINT32_C(0x0100), UINT16_C(0x0000), UINT8_C(0x03),
                     UINT16_C(0x0007), UINT16_C(0xFFFF));
}

static void
check_mmr_rejects(uint16_t sp_before)
{
    fcall_fixture fixture = {
        .words = {(uint16_t)(FCALL_WORD0_BASE | UINT16_C(0x01)),
                  UINT16_C(0x3333)},
        .program_calls = 0U,
        .addresses = {0U, 0U},
        .fail_second = false,
        .data_wait_then_ready = false,
        .data_fault = false,
        .data_waited = false,
        .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    ti_c54x_run_result result;

    seed_cpu(cpu, sp_before, UINT16_C(0x0000));
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
test_mmr_tos_rejects(void)
{
    /* Old SP 0001 puts the first TOS at 0000 (IMR): an
     * MMR-routed first TOS has no endpoint first WRITE and
     * MMR-1 write semantics are unevidenced. */
    check_mmr_rejects(UINT16_C(0x0001));
}

static void
test_mmr_second_tos_rejects(void)
{
    /* Old SP 0002 puts the second TOS at 0000 (IMR) behind an
     * endpoint first TOS: the register would serve instead of
     * the endpoint, so the push rejects the same way. */
    check_mmr_rejects(UINT16_C(0x0002));
}

static void
test_frozen_xpc_rides_second_push(void)
{
    fcall_fixture fixture = {
        .words = {(uint16_t)(FCALL_WORD0_BASE | UINT16_C(0x02)),
                  UINT16_C(0x2000)},
        .program_calls = 0U,
        .addresses = {0U, 0U},
        .fail_second = false,
        .data_wait_then_ready = true,
        .data_fault = false,
        .data_waited = false,
        .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x0005));
    /* WAIT parks after the first TOS WRITE issues: nothing is
     * published yet. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x1111));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0100));
    TI_C54X_TEST_CHECK(fixture.data_calls == 1U);
    /* A mid-flight XPC change cannot divert the frozen second
     * push: only far control modifies XPC and none retires while
     * the transaction is pending. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_xpc(&cpu->arch_state,
                                                   UINT16_C(0x0042)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.data_calls == 2U);
    check_write_request(&fixture.data_requests[0], UINT16_C(0x1110),
                        UINT16_C(0x0102));
    check_write_request(&fixture.data_requests[1], UINT16_C(0x110F),
                        UINT16_C(0x0005));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x110F));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_wait_resumes_with_atomic_commit(void)
{
    fcall_fixture fixture = {
        .words = {(uint16_t)(FCALL_WORD0_BASE | UINT16_C(0x01)),
                  UINT16_C(0x3333)},
        .program_calls = 0U,
        .addresses = {0U, 0U},
        .fail_second = false,
        .data_wait_then_ready = true,
        .data_fault = false,
        .data_waited = false,
        .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x0000));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x1111));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0025));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.data_calls == 2U);
    check_write_request(&fixture.data_requests[0], UINT16_C(0x1110),
                        UINT16_C(0x0027));
    check_write_request(&fixture.data_requests[1], UINT16_C(0x110F),
                        UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x110F));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x3333));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_data_fault_is_transactional(void)
{
    fcall_fixture fixture = {
        .words = {(uint16_t)(FCALL_WORD0_BASE | UINT16_C(0x01)),
                  UINT16_C(0x3333)},
        .program_calls = 0U,
        .addresses = {0U, 0U},
        .fail_second = false,
        .data_wait_then_ready = false,
        .data_fault = true,
        .data_waited = false,
        .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x0000));
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
    fcall_fixture fixture = {
        .words = {(uint16_t)(FCALL_WORD0_BASE | UINT16_C(0x01)),
                  UINT16_C(0x3333)},
        .program_calls = 0U,
        .addresses = {0U, 0U},
        .fail_second = false,
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
    fcall_fixture fixture = {
        .words = {(uint16_t)(FCALL_WORD0_BASE | UINT16_C(0x01)),
                  UINT16_C(0x3333)},
        .program_calls = 0U,
        .addresses = {0U, 0U},
        .fail_second = false,
        .data_wait_then_ready = false,
        .data_fault = false,
        .data_waited = false,
        .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x0000));
    /* Clear XPC knownness: the second push has no bonus value. */
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

static void
test_second_word_fault_is_transactional(void)
{
    fcall_fixture fixture = {
        .words = {(uint16_t)(FCALL_WORD0_BASE | UINT16_C(0x01)),
                  UINT16_C(0x3333)},
        .program_calls = 0U,
        .addresses = {0U, 0U},
        .fail_second = true,
        .data_wait_then_ready = false,
        .data_fault = false,
        .data_waited = false,
        .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x0000));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
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
    test_origins_and_targets();
    test_mmr_tos_rejects();
    test_mmr_second_tos_rejects();
    test_frozen_xpc_rides_second_push();
    test_wait_resumes_with_atomic_commit();
    test_data_fault_is_transactional();
    test_unknown_sp_rejects();
    test_unknown_xpc_rejects();
    test_second_word_fault_is_transactional();
    puts("cpu_fcall: ok");
    return 0;
}
