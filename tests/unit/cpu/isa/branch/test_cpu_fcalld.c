/* Module contract
 * Owns: public-CPU RED proofs for the two-word `FCALLD pmad`
 * delayed far call, including the TI worked-example vector with
 * the double-push TOS capture (return address then live XPC),
 * sequential advance with countdown arming, deferred-branch
 * take-effect, WAIT resume with atomic commit, DATA fault and
 * unknown-SP/XPC transactionality, and full architectural
 * preservation outside SP/PC/XPC/RTN.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct fcalld_fixture {
    uint16_t words[4];
    unsigned program_calls;
    unsigned word_count;
    ti_c54x_program_word_address base;
    bool data_wait_then_ready;
    bool data_fault;
    bool data_waited;
    ti_c54x_bus_word16_request data_requests[2];
    unsigned data_calls;
} fcalld_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    fcalld_fixture *fixture = (fcalld_fixture *)context;
    ti_c54x_program_word_address offset;

    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address >= fixture->base);
    offset = address - fixture->base;
    TI_C54X_TEST_CHECK(offset < fixture->word_count);
    *value = fixture->words[offset];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    fcalld_fixture *fixture = (fcalld_fixture *)context;

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
init_cpu(fcalld_fixture *fixture, unsigned char *storage,
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

/* FORM-T2-13-05: 1111_1011_1PPP_PPPP with Z=1; word1 is the copied
 * raw pmad. */
static const uint16_t FCALLD_WORD0_BASE = UINT16_C(0xFB80);
static const uint16_t NOP_WORD0 = UINT16_C(0xF495);

static void
seed_cpu(ti_c54x_cpu *cpu, uint16_t sp_value, uint16_t xpc_value)
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
    fcalld_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    const uint16_t expected_second_tos =
        (uint16_t)(sp_before - UINT16_C(2));
    const uint16_t expected_return =
        (uint16_t)(start_pc + UINT32_C(4));

    fixture.base = start_pc;
    fixture.words[0] = (uint16_t)(FCALLD_WORD0_BASE | page);
    fixture.words[1] = pmad;
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_cpu(cpu, sp_before, xpc_before);

    /* FCALLD retires first: both TOS WRITEs issue at once with
     * the PC+4 return address and the frozen live XPC, PC
     * advances sequentially, XPC commits with the arming
     * retirement, and the countdown arms with pmad deferred. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == fixture.words[0]);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_second_tos);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word == pmad);
    TI_C54X_TEST_CHECK(fixture.data_calls == 2U);
    check_write_request(&fixture.data_requests[0],
                        (uint16_t)(sp_before - UINT16_C(1)),
                        expected_return);
    check_write_request(&fixture.data_requests[1], expected_second_tos,
                        xpc_before);
    {
        uint16_t xpc = UINT16_C(0xFFFF);

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(
                               &cpu->arch_state, &xpc) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(xpc == (uint16_t)page);
    }

    /* Both delay slots retire, then the deferred branch takes
     * effect with the armed XPC already committed. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_second_tos);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)pmad);
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_worked_example(void)
{
    /* TI SPRU172C FCALLD 301000h example: PC 3001 and SP 1111
     * become TOS[1110] 3005 and TOS[110F] 007F with SP 110F,
     * then PC 1000 with XPC 30 after the slots. */
    check_ready_case(UINT32_C(0x3001), UINT16_C(0x1111), UINT8_C(0x30),
                     UINT16_C(0x007F), UINT16_C(0x1000));
}

static void
test_origins_and_targets(void)
{
    check_ready_case(UINT32_C(0x0000), UINT16_C(0x0021), UINT8_C(0x00),
                     UINT16_C(0x0000), UINT16_C(0x0000));
    check_ready_case(UINT32_C(0x1F45), UINT16_C(0x8000), UINT8_C(0x7F),
                     UINT16_C(0x0012), UINT16_C(0x2000));
}

static void
test_wait_resumes_armed(void)
{
    fcalld_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x0025);
    fixture.words[0] = (uint16_t)(FCALLD_WORD0_BASE | UINT16_C(0x01));
    fixture.words[1] = UINT16_C(0x3333);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x0000));
    fixture.data_wait_then_ready = true;

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x1111));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.data_calls == 2U);
    check_write_request(&fixture.data_requests[0], UINT16_C(0x1110),
                        UINT16_C(0x0029));
    check_write_request(&fixture.data_requests[1], UINT16_C(0x110F),
                        UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_data_fault_is_transactional(void)
{
    fcalld_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x0025);
    fixture.words[0] = (uint16_t)(FCALLD_WORD0_BASE | UINT16_C(0x01));
    fixture.words[1] = UINT16_C(0x3333);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x0000));
    fixture.data_fault = true;
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(
            !ti_c54x_sequencer_delayed_branch_is_armed(
                &cpu->delayed_branch));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_unknown_sp_rejects(void)
{
    fcalld_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x0025);
    fixture.words[0] = (uint16_t)(FCALLD_WORD0_BASE | UINT16_C(0x01));
    fixture.words[1] = UINT16_C(0x3333);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
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
    fcalld_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x0025);
    fixture.words[0] = (uint16_t)(FCALLD_WORD0_BASE | UINT16_C(0x01));
    fixture.words[1] = UINT16_C(0x3333);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x0000));
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
    test_origins_and_targets();
    test_wait_resumes_armed();
    test_data_fault_is_transactional();
    test_unknown_sp_rejects();
    test_unknown_xpc_rejects();
    puts("cpu_fcalld: ok");
    return 0;
}
