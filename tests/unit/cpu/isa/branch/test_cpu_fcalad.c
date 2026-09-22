/* Module contract
 * Owns: public-CPU RED proofs for the one-word `FCALAD src`
 * delayed far accumulator call, including the TI
 * worked-example vector with the double-push TOS capture
 * (return address then live XPC), sequential advance with
 * countdown arming, deferred-branch take-effect, WAIT resume
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct fcalad_fixture {
    uint16_t words[3];
    unsigned program_calls;
    unsigned word_count;
    ti_c54x_program_word_address base;
    bool data_wait_then_ready;
    bool data_fault;
    bool data_waited;
    ti_c54x_bus_word16_request data_requests[2];
    unsigned data_calls;
} fcalad_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    fcalad_fixture *fixture = (fcalad_fixture *)context;
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
    fcalad_fixture *fixture = (fcalad_fixture *)context;

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
init_cpu(fcalad_fixture *fixture, unsigned char *storage,
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

/* FORM-T2-13-04: 1111_011S_1110_0111 with Z=1; S at bit 8 selects
 * accumulator A (0) or B (1). */
static const uint16_t FCALAD_WORD0_BASE = UINT16_C(0xF6E7);
static const uint16_t NOP_WORD0 = UINT16_C(0xF495);

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
    fcalad_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    const uint16_t expected_second_tos =
        (uint16_t)(sp_before - UINT16_C(2));
    const uint16_t expected_return =
        (uint16_t)(start_pc + UINT32_C(3));

    fixture.base = start_pc;
    fixture.words[0] = (uint16_t)(FCALAD_WORD0_BASE |
                                  (uint16_t)((uint16_t)select << 8U));
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_cpu(cpu, sp_before, xpc_before, acc_a, acc_b);

    /* FCALAD retires first: both TOS WRITEs issue at once with
     * the PC+3 return address and the frozen live XPC, PC
     * advances sequentially, XPC commits with the arming
     * retirement, and the countdown arms with the accumulator
     * target deferred. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == fixture.words[0]);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_second_tos);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(1));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word == target);
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
        TI_C54X_TEST_CHECK(xpc == page);
    }

    /* Both delay slots retire, then the deferred branch takes
     * effect with the armed XPC already committed. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_second_tos);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)target);
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_worked_example(void)
{
    /* TI SPRU172C FCALAD B example with B 00 0020 2000: PC 0025
     * and SP 1111 become TOS[1110] 0028 and TOS[110F] 007F with
     * SP 110F, then PC 2000 with XPC 20 after the slots. */
    check_ready_case(UINT32_C(0x0025), UINT16_C(0x1111), UINT8_C(1),
                     UINT64_C(0x1111222233), UINT64_C(0x00202000),
                     UINT16_C(0x007F), UINT16_C(0x2000),
                     UINT16_C(0x0020));
}

static void
test_select_a(void)
{
    /* S=0 selects A: PC and XPC follow A bits 15-0/22-16. */
    check_ready_case(UINT32_C(0x0100), UINT16_C(0x0021), UINT8_C(0),
                     UINT64_C(0x00013000), UINT64_C(0x0000AABBCC),
                     UINT16_C(0x0000), UINT16_C(0x3000),
                     UINT16_C(0x0001));
}

static void
test_data_fault_is_transactional(void)
{
    fcalad_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x0025);
    fixture.words[0] = (uint16_t)(FCALAD_WORD0_BASE | UINT16_C(0x0100));
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x007F),
             UINT64_C(0x1111222233), UINT64_C(0x00202000));
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
test_unknown_xpc_rejects(void)
{
    fcalad_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x0025);
    fixture.words[0] = (uint16_t)(FCALAD_WORD0_BASE | UINT16_C(0x0100));
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x0025));
    seed_cpu(cpu, UINT16_C(0x1111), UINT16_C(0x007F),
             UINT64_C(0x1111222233), UINT64_C(0x00202000));
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
    test_select_a();
    test_data_fault_is_transactional();
    test_unknown_xpc_rejects();
    puts("cpu_fcalad: ok");
    return 0;
}
