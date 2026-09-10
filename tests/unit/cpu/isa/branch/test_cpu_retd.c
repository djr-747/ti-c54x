/* Module contract
 * Owns: public-CPU RED proofs for the one-word `RETD`
 * delayed subroutine return, including the two-slot countdown
 * with sequential fetch, deferred-branch take-effect, one- and
 * two-word slot fills, inner-branch takeover, oversize-fill
 * saturation, WAIT resume, budget-split persistence, reset
 * clearing, slot-fetch faulting, unknown-SP transactionality,
 * and full architectural preservation outside SP/PC.
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

typedef struct retd_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
    bool program_fault;
    uint32_t fault_address;
    /* DATA endpoint script: WAIT once, then READY with tos; or FAULT. */
    bool data_wait_then_ready;
    bool data_fault;
    bool data_waited;
    uint16_t tos_value;
    ti_c54x_bus_word16_request data_request;
    unsigned data_calls;
} retd_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    retd_fixture *fixture = (retd_fixture *)context;
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
    retd_fixture *fixture = (retd_fixture *)context;

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
init_cpu(retd_fixture *fixture, unsigned char *storage, uint32_t reset_pc)
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

/* Table 2-15 syntax 4: 1111_11Z0_0000_0000 with Z=1. */
static const uint16_t RETD_WORD0 = UINT16_C(0xFE00);
static const uint16_t NOP_WORD0 = UINT16_C(0xF495);
/* LD #lk,A: 0xF062 with destination bit 8 clear selects A. */
static const uint16_t LD_LK_A_WORD0 = UINT16_C(0xF062);
/* B pmad: 0xF073 followed by the 16-bit target word. */
static const uint16_t B_WORD0 = UINT16_C(0xF073);

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

static retd_fixture
make_fixture(uint32_t base, uint16_t tos_value)
{
    retd_fixture fixture = {0};

    fixture.base = base;
    fixture.tos_value = tos_value;
    return fixture;
}

static void
test_two_one_word_slots(void)
{
    retd_fixture fixture = make_fixture(UINT32_C(0x2112),
                                        UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = RETD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));

    /* RETD retires first: SP commits at once, PC advances
     * sequentially, and the countdown arms with TOS deferred. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == RETD_WORD0);
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0301));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2113));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word ==
                       UINT16_C(0x1000));
    TI_C54X_TEST_CHECK(fixture.data_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                       UINT16_C(0x0300));

    /* Both delay slots retire, then the deferred branch takes effect.
     * Run counters are per call: two attempts, two retirements here. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0301));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_one_two_word_slot(void)
{
    retd_fixture fixture = make_fixture(UINT32_C(0x2112),
                                        UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_accumulator_snapshot before_b;
    ti_c54x_accumulator_snapshot after_a;
    ti_c54x_accumulator_snapshot after_b;

    /* One 2-word LD fills both delay slots and must really execute. */
    fixture.words[0] = RETD_WORD0;
    fixture.words[1] = LD_LK_A_WORD0;
    fixture.words[2] = UINT16_C(0x1234);
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));
    seed_sxm(cpu, UINT8_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_snapshot(
                           &cpu->accumulators, &before_b) ==
                       TI_C54X_ACCUMULATOR_OK);

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0301));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
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
    retd_fixture fixture = make_fixture(UINT32_C(0x2112),
                                        UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    /* A branch inside the delay slots is TI-forbidden; the inner
     * redirect deterministically cancels the pending countdown. */
    fixture.words[0] = RETD_WORD0;
    fixture.words[1] = B_WORD0;
    fixture.words[2] = UINT16_C(0x2000);
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0301));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_oversize_slot_still_branches(void)
{
    retd_fixture fixture = make_fixture(UINT32_C(0x2112),
                                        UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    /* One word left with a two-word instruction retiring: the
     * malformed fill still takes the branch. */
    fixture.words[0] = RETD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = LD_LK_A_WORD0;
    fixture.words[3] = UINT16_C(0x1234);
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));
    seed_sxm(cpu, UINT8_C(0));

    result = ti_c54x_cpu_run(cpu, UINT64_C(3));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(3));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(3));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_wait_resumes_armed(void)
{
    retd_fixture fixture = make_fixture(UINT32_C(0x2112),
                                        UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = RETD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    fixture.data_wait_then_ready = true;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));

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
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0301));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_budget_split_drains(void)
{
    retd_fixture fixture = make_fixture(UINT32_C(0x2112),
                                        UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = RETD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));

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
test_reset_clears_countdown(void)
{
    retd_fixture fixture = make_fixture(UINT32_C(0x2112),
                                        UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = RETD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(0));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2112));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_program_fault_in_slot(void)
{
    retd_fixture fixture = make_fixture(UINT32_C(0x2112),
                                        UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = RETD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    fixture.program_fault = true;
    fixture.fault_address = UINT32_C(0x2113);
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* The faulting slot never retired, so the countdown holds. */
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0301));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2113));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_sp_rejects(void)
{
    retd_fixture fixture = make_fixture(UINT32_C(0x2112),
                                        UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = RETD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));

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
        TI_C54X_TEST_CHECK(
            !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_architectural_preservation(void)
{
    retd_fixture fixture = make_fixture(UINT32_C(0x2112),
                                        UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = RETD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;
        const ti_c54x_t_register_state before_t = cpu->t_register;
        ti_c54x_arch_state expect_arch;

        result = ti_c54x_cpu_run(cpu, UINT64_C(3));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(3));
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                  sizeof(before_t)) == 0);

        /* Only SP and the program counter may move. */
        expect_arch = before_arch;
        expect_arch.pc_word = UINT32_C(0x1000);
        expect_arch.sp = UINT16_C(0x0301);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &expect_arch,
                                  sizeof(expect_arch)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    test_two_one_word_slots();
    test_one_two_word_slot();
    test_inner_branch_wins();
    test_oversize_slot_still_branches();
    test_wait_resumes_armed();
    test_budget_split_drains();
    test_reset_clears_countdown();
    test_program_fault_in_slot();
    test_unknown_sp_rejects();
    test_architectural_preservation();
    puts("cpu_retd: ok");
    return 0;
}
