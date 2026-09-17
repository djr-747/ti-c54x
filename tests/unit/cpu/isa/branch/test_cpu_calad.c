/* Module contract
 * Owns: public-CPU RED proofs for the one-word `CALAD`
 * accumulator-addressed delayed subroutine call, including the
 * CALAD worked-example vector with PC+3 return-address capture,
 * the two-slot countdown with sequential fetch, deferred-branch
 * take-effect from the frozen accumulator target, one- and
 * two-word slot fills, inner-branch takeover, WAIT resume with
 * atomic commit, DATA fault and unknown-SP/unknown-accumulator
 * transactionality, one-word fetch ordering, and full
 * architectural preservation outside SP/PC.
 * Does not own: production CPU behavior, catalog/decode or pure
 * call effects, address planning, shared fixtures, timing,
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

typedef struct calad_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
    bool program_fault;
    uint32_t fault_address;
    /* DATA endpoint script: WAIT once, then READY; or FAULT. */
    bool data_wait_then_ready;
    bool data_fault;
    bool data_waited;
    ti_c54x_bus_word16_request data_request;
    unsigned data_calls;
} calad_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    calad_fixture *fixture = (calad_fixture *)context;
    uint32_t offset;

    TI_C54X_TEST_CHECK(value != NULL);
    if (fixture->program_fault && address == fixture->fault_address) {
        return TI_C54X_BUS_FAULT;
    }
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
    calad_fixture *fixture = (calad_fixture *)context;

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
init_cpu(calad_fixture *fixture, unsigned char *storage,
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

/* FORM-T2-13-01: 1111_01Z1_1110_0011 with Z=1; S at bit 8 selects
 * accumulator A (0) or B (1). */
static const uint16_t CALAD_WORD0 = UINT16_C(0xF6E3);
static const uint16_t NOP_WORD0 = UINT16_C(0xF495);
static const uint16_t LD_LK_A_WORD0 = UINT16_C(0xF062);
static const uint16_t B_WORD0 = UINT16_C(0xF073);

static void
seed_cpu(ti_c54x_cpu *cpu, uint16_t sp_value, uint64_t acc_a,
         uint64_t acc_b)
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
    /* CMPT/ARP stay unknown: pushes are mode-independent end to end. */
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
seed_sxm(ti_c54x_cpu *cpu, uint8_t sxm)
{
    TI_C54X_TEST_CHECK(sxm <= UINT8_C(1));
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 &
                   (uint16_t)~TI_C54X_ST1_SXM_MASK);
    if (sxm != UINT8_C(0)) {
        cpu->arch_state.status.st1 =
            (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_SXM_MASK);
    }
}

static calad_fixture
make_fixture(uint32_t base)
{
    calad_fixture fixture = {0};

    fixture.base = base;
    return fixture;
}

static void
check_ready_case(uint32_t start_pc, uint16_t sp_before, uint8_t select,
                 uint64_t acc_a, uint64_t acc_b, uint16_t target)
{
    calad_fixture fixture = make_fixture(start_pc);
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    const uint16_t expected_tos = (uint16_t)(sp_before - UINT16_C(1));
    const uint16_t expected_return = (uint16_t)(start_pc + UINT32_C(3));

    fixture.words[0] = (uint16_t)(CALAD_WORD0 |
                                  (uint16_t)((uint16_t)select << 8U));
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_cpu(cpu, sp_before, acc_a, acc_b);

    /* CALAD retires first: SP commits at once with the PC+3 TOS
     * WRITE, PC advances sequentially, and the countdown arms with
     * the accumulator target deferred. Run counters are per call. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == fixture.words[0]);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_tos);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(1));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word == target);
    /* The TOS WRITE carries PC+3 at old SP - 1. */
    TI_C54X_TEST_CHECK(fixture.data_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.data_request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.data_request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                       expected_tos);
    TI_C54X_TEST_CHECK(fixture.data_request.write_value_word ==
                       expected_return);

    /* Both delay slots retire, then the deferred branch takes effect. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_tos);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)target);
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_worked_example(void)
{
    /* TI SPRU172C CALAD B example: B 00 0000 2000 with PC 0025 and
     * SP 1111 becomes TOS[1110] 0028 and SP 1110, with PC 2000
     * after the slots. */
    check_ready_case(UINT32_C(0x0025), UINT16_C(0x1111), UINT8_C(1),
                     UINT64_C(0x00000000), UINT64_C(0x00002000),
                     UINT16_C(0x2000));
}

static void
test_origins_targets_and_selects(void)
{
    check_ready_case(UINT32_C(0x0000), UINT16_C(0x0021), UINT8_C(0),
                     UINT64_C(0x00000000), UINT64_C(0x00000000),
                     UINT16_C(0x0000));
    check_ready_case(UINT32_C(0x1F45), UINT16_C(0x8000), UINT8_C(1),
                     UINT64_C(0x1111222233), UINT64_C(0x00004567),
                     UINT16_C(0x4567));
    check_ready_case(UINT32_C(0x0100), UINT16_C(0x0000), UINT8_C(0),
                     UINT64_C(0xFF00FFFFFF), UINT64_C(0x00000000),
                     UINT16_C(0xFFFF));
}

static void
test_one_two_word_slot(void)
{
    calad_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_accumulator_snapshot before_b;
    ti_c54x_accumulator_snapshot after_a;
    ti_c54x_accumulator_snapshot after_b;

    /* One 2-word LD fills both delay slots and must really execute;
     * the deferred target stays frozen from CALAD retirement. */
    fixture.words[0] = CALAD_WORD0;
    fixture.words[1] = LD_LK_A_WORD0;
    fixture.words[2] = UINT16_C(0x1234);
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300), UINT64_C(0x00001000),
             UINT64_C(0x00000000));
    seed_sxm(cpu, UINT8_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_snapshot(
                           &cpu->accumulators, &before_b) ==
                       TI_C54X_ACCUMULATOR_OK);

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x02FF));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    /* The TOS WRITE carries PC+3 (0x2115) at old SP - 1. */
    TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                       UINT16_C(0x02FF));
    TI_C54X_TEST_CHECK(fixture.data_request.write_value_word ==
                       UINT16_C(0x2115));
    /* SXM=0 zero-extends 0x1234 and shifts left 16 into A. */
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_snapshot(
                           &cpu->accumulators, &after_a) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(after_a.a_raw40 == UINT64_C(0x12340000));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_snapshot(
                           &cpu->accumulators, &after_b) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(after_b.b_raw40 == before_b.b_raw40);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_inner_branch_wins(void)
{
    calad_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    /* A branch inside the delay slots is TI-forbidden; the inner
     * redirect deterministically cancels the pending countdown. */
    fixture.words[0] = CALAD_WORD0;
    fixture.words[1] = B_WORD0;
    fixture.words[2] = UINT16_C(0x2000);
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300), UINT64_C(0x00001000),
             UINT64_C(0x00000000));

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x02FF));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_wait_resumes_armed(void)
{
    calad_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALAD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    fixture.data_wait_then_ready = true;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300), UINT64_C(0x00001000),
             UINT64_C(0x00000000));

    result = ti_c54x_cpu_run(cpu, UINT64_C(3));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0300));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2112));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));

    result = ti_c54x_cpu_run(cpu, UINT64_C(3));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(3));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x02FF));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_budget_split_drains(void)
{
    calad_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALAD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300), UINT64_C(0x00001000),
             UINT64_C(0x00000000));

    /* Retirement counts are per call: one instruction per step. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2113));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2114));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(1));

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_program_fault_in_slot(void)
{
    calad_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALAD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    fixture.program_fault = true;
    fixture.fault_address = UINT32_C(0x2113);
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300), UINT64_C(0x00001000),
             UINT64_C(0x00000000));

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* The faulting slot never retired, so the countdown holds. */
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x02FF));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2113));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_sp_rejects(void)
{
    calad_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALAD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));

    /* Seed everything except SP: reset leaves SP unknown. */
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x00001000)) ==
                       TI_C54X_ACCUMULATOR_OK);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(
            !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_accumulator_rejects(void)
{
    calad_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALAD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));

    /* Seed SP but leave the accumulators unknown: reset leaves
     * accumulator knownness clear. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  UINT16_C(0x0300)) ==
                       TI_C54X_ARCH_STATE_OK);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(
            !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_data_fault_is_transactional(void)
{
    calad_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALAD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    fixture.data_fault = true;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300), UINT64_C(0x00001000),
             UINT64_C(0x00000000));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(
            !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
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
    test_origins_targets_and_selects();
    test_one_two_word_slot();
    test_inner_branch_wins();
    test_wait_resumes_armed();
    test_budget_split_drains();
    test_program_fault_in_slot();
    test_unknown_sp_rejects();
    test_unknown_accumulator_rejects();
    test_data_fault_is_transactional();
    return 0;
}
