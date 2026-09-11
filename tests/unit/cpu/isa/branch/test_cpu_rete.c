/* Module contract
 * Owns: public-CPU RED proofs for the one-word `RETE`
 * interrupt return, including the worked-example vector with TOS
 * capture, WAIT resume with atomic commit, DATA fault and
 * unknown-SP transactionality, single-word fetch ordering, and
 * full architectural preservation outside
 * SP/PC/INTM.
 * Does not own: production CPU behavior, catalog/decode or pure
 * return effects, address planning, shared fixtures, timing,
 * diagnostics, or test registration outside the branch leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct rete_fixture {
    uint16_t word;
    unsigned program_calls;
    ti_c54x_program_word_address address;
    bool program_fault;
    /* DATA endpoint script: WAIT once, then READY with tos; or FAULT. */
    bool data_wait_then_ready;
    bool data_fault;
    bool data_waited;
    uint16_t tos_value;
    ti_c54x_bus_word16_request data_request;
    unsigned data_calls;
} rete_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    rete_fixture *fixture = (rete_fixture *)context;

    TI_C54X_TEST_CHECK(fixture->program_calls == 0U);
    TI_C54X_TEST_CHECK(value != NULL);
    if (fixture->program_fault) {
        return TI_C54X_BUS_FAULT;
    }
    fixture->address = address;
    *value = fixture->word;
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    rete_fixture *fixture = (rete_fixture *)context;

    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->data_request = *request;
    fixture->data_calls++;
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
    observation->read_value_word = fixture->tos_value;
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
init_cpu(rete_fixture *fixture, unsigned char *storage, uint32_t reset_pc)
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

/* Table 2-15 syntax 5 (SPRU172C p. 4-139): 1111_01Z0_1110_1011
 * with Z=0. */
static const uint16_t RETE_WORD0 = UINT16_C(0xF4EB);

static void
seed_cpu(ti_c54x_cpu *cpu, uint16_t sp_value)
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
    /* CMPT/ARP stay unknown: pops are mode-independent end to end. */
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
check_ready_case(uint32_t start_pc, uint16_t sp_before, uint16_t tos_value)
{
    rete_fixture fixture = {.word = RETE_WORD0,
                           .program_calls = 0U,
                           .address = 0U,
                           .program_fault = false,
                           .data_wait_then_ready = false,
                           .data_fault = false,
                           .data_waited = false,
                           .tos_value = tos_value,
                           .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, start_pc);
    ti_c54x_run_result result;
    ti_c54x_state state;
    const uint16_t expected_sp = (uint16_t)(sp_before + UINT16_C(1));

    seed_cpu(cpu, sp_before);
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
        TI_C54X_TEST_CHECK(result.opcode_word == RETE_WORD0);
        TI_C54X_TEST_CHECK(result.opcode_valid);
        TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
        TI_C54X_TEST_CHECK(fixture.address == start_pc);
        /* The TOS READ issues at old SP and returns the PC target. */
        TI_C54X_TEST_CHECK(fixture.data_calls == 1U);
        TI_C54X_TEST_CHECK(fixture.data_request.space ==
                           TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
        TI_C54X_TEST_CHECK(fixture.data_request.access ==
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
        TI_C54X_TEST_CHECK(fixture.data_request.address_word == sp_before);
        TI_C54X_TEST_CHECK(result.pc_word == (uint32_t)tos_value);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)tos_value);
        TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_sp);
        /* The seed sets ST1 bit 11 (INTM); RETE clears exactly it. */
        TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                           UINT16_C(0x0800)) == UINT16_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == UINT16_C(0x525A));
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st1_known_mask ==
                           UINT16_C(0xFFFF));
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                  sizeof(before_t)) == 0);

        /* Only SP, the program counter, and INTM may move. */
        {
            ti_c54x_arch_state expect_arch = before_arch;

            expect_arch.pc_word = (uint32_t)tos_value;
            expect_arch.sp = expected_sp;
            expect_arch.status.st1 = UINT16_C(0x525A);
            TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &expect_arch,
                                      sizeof(expect_arch)) == 0);
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == (uint32_t)tos_value);
        TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_worked_example(void)
{
    /* TI SPRU172C RETE example (p. 4-139): PC 01C3 and SP 2001
     * with TOS 0110 become PC 0110 and SP 2002 with ST1 bit 11
     * (INTM) cleared. */
    check_ready_case(UINT32_C(0x01C3), UINT16_C(0x2001),
                     UINT16_C(0x0110));
}

static void
test_origins_and_targets(void)
{
    check_ready_case(UINT32_C(0x0000), UINT16_C(0x0000),
                     UINT16_C(0x0000));
    check_ready_case(UINT32_C(0x1F45), UINT16_C(0x8000),
                     UINT16_C(0x2000));
    check_ready_case(UINT32_C(0x0100), UINT16_C(0xffff),
                     UINT16_C(0x00a5));
}

static void
test_wait_resumes_with_atomic_commit(void)
{
    rete_fixture fixture = {.word = RETE_WORD0,
                           .program_calls = 0U,
                           .address = 0U,
                           .program_fault = false,
                           .data_wait_then_ready = true,
                           .data_fault = false,
                           .data_waited = false,
                           .tos_value = UINT16_C(0x1000),
                           .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x0300));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0300));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2112));
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == UINT16_C(0x5A5A));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0301));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == UINT16_C(0x525A));
    TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                       UINT16_C(0x0300));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_data_fault_is_transactional(void)
{
    rete_fixture fixture = {.word = RETE_WORD0,
                           .program_calls = 0U,
                           .address = 0U,
                           .program_fault = false,
                           .data_wait_then_ready = false,
                           .data_fault = true,
                           .data_waited = false,
                           .tos_value = UINT16_C(0x1000),
                           .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x0300));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_sp_rejects(void)
{
    rete_fixture fixture = {.word = RETE_WORD0,
                           .program_calls = 0U,
                           .address = 0U,
                           .program_fault = false,
                           .data_wait_then_ready = false,
                           .data_fault = false,
                           .data_waited = false,
                           .tos_value = UINT16_C(0x1000),
                           .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    ti_c54x_run_result result;

    /* Seed everything except SP: reset leaves SP unknown. */
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_program_fault_is_transactional(void)
{
    rete_fixture fixture = {.word = RETE_WORD0,
                           .program_calls = 0U,
                           .address = 0U,
                           .program_fault = true,
                           .data_wait_then_ready = false,
                           .data_fault = false,
                           .data_waited = false,
                           .tos_value = UINT16_C(0x1000),
                           .data_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x0300));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    test_worked_example();
    test_origins_and_targets();
    test_wait_resumes_with_atomic_commit();
    test_data_fault_is_transactional();
    test_unknown_sp_rejects();
    test_program_fault_is_transactional();
    return 0;
}
