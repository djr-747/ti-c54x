/* Module contract
 * Owns: public-CPU RED proofs for the two-word `CALLD`
 * delayed subroutine call, including the CALLD worked-example
 * vector with PC+4 return-address capture, the two-slot
 * countdown with sequential fetch, deferred-branch take-effect,
 * one- and two-word slot fills, inner-branch takeover,
 * oversize-fill saturation, WAIT resume with atomic commit, DATA
 * fault and unknown-SP transactionality, two-word fetch
 * ordering, and full architectural preservation outside SP/PC.
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
#include "../../../../support/test_arch_state_compare.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct calld_fixture {
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
} calld_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    calld_fixture *fixture = (calld_fixture *)context;
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
    calld_fixture *fixture = (calld_fixture *)context;

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
init_cpu(calld_fixture *fixture, unsigned char *storage,
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

/* FORM-T2-13-02: 1111_00Z0_0111_0100 with Z=1; word1 is the copied
 * raw pmad. */
static const uint16_t CALLD_WORD0 = UINT16_C(0xF274);
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

static calld_fixture
make_fixture(uint32_t base)
{
    calld_fixture fixture = {0};

    fixture.base = base;
    return fixture;
}

static void
check_ready_case(uint32_t start_pc, uint16_t sp_before, uint16_t pmad)
{
    calld_fixture fixture = make_fixture(start_pc);
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    const uint16_t expected_tos = (uint16_t)(sp_before - UINT16_C(1));
    const uint16_t expected_return = (uint16_t)(start_pc + UINT32_C(4));

    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = pmad;
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_cpu(cpu, sp_before);

    /* CALLD retires first: SP commits at once with the PC+4 TOS
     * WRITE, PC advances sequentially, and the countdown arms with
     * pmad deferred. Run counters are per call. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == CALLD_WORD0);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_tos);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word == pmad);
    /* The TOS WRITE carries PC+4 at old SP - 1. */
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
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)pmad);
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_worked_example(void)
{
    /* TI SPRU172C CALLD 1000h example: PC 0025 and SP 1111 become
     * TOS[1110] 0029 and SP 1110, with PC 1000 after the slots. */
    check_ready_case(UINT32_C(0x0025), UINT16_C(0x1111),
                     UINT16_C(0x1000));
}

static void
test_origins_and_targets(void)
{
    check_ready_case(UINT32_C(0x0000), UINT16_C(0x0021),
                     UINT16_C(0x0000));
    check_ready_case(UINT32_C(0x1F45), UINT16_C(0x8000),
                     UINT16_C(0x2000));
    check_ready_case(UINT32_C(0x0100), UINT16_C(0x0000),
                     UINT16_C(0xFFFF));
}

static void
test_one_two_word_slot(void)
{
    calld_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_accumulator_snapshot before_b;
    ti_c54x_accumulator_snapshot after_a;
    ti_c54x_accumulator_snapshot after_b;

    /* One 2-word LD fills both delay slots and must really execute. */
    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = UINT16_C(0x1000);
    fixture.words[2] = LD_LK_A_WORD0;
    fixture.words[3] = UINT16_C(0x1234);
    fixture.word_count = 4U;
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
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x02FF));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    /* The TOS WRITE carries PC+4 (0x2116) at old SP - 1. */
    TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                       UINT16_C(0x02FF));
    TI_C54X_TEST_CHECK(fixture.data_request.write_value_word ==
                       UINT16_C(0x2116));
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
    calld_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    /* A branch inside the delay slots is TI-forbidden; the inner
     * redirect deterministically cancels the pending countdown. */
    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = UINT16_C(0x1000);
    fixture.words[2] = B_WORD0;
    fixture.words[3] = UINT16_C(0x2000);
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));

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
test_oversize_slot_still_branches(void)
{
    calld_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    /* One word left with a two-word instruction retiring: the
     * malformed fill still takes the branch. */
    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = UINT16_C(0x1000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = LD_LK_A_WORD0;
    fixture.words[4] = UINT16_C(0x1234);
    fixture.word_count = 5U;
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
    calld_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = UINT16_C(0x1000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
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
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x02FF));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_budget_split_drains(void)
{
    calld_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = UINT16_C(0x1000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));

    /* Retirement counts are per call: one instruction per step. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2114));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2115));
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
    calld_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = UINT16_C(0x1000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
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
    calld_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = UINT16_C(0x1000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    fixture.program_fault = true;
    fixture.fault_address = UINT32_C(0x2114);
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* The faulting slot never retired, so the countdown holds. */
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x02FF));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2114));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_sp_rejects(void)
{
    calld_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = UINT16_C(0x1000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
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
test_data_fault_is_transactional(void)
{
    calld_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = UINT16_C(0x1000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    fixture.data_fault = true;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu, UINT16_C(0x0300));
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

static void
test_second_word_fault_is_transactional(void)
{
    calld_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = UINT16_C(0x1000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    fixture.program_fault = true;
    fixture.fault_address = UINT32_C(0x2113);
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
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
test_architectural_preservation(void)
{
    calld_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = CALLD_WORD0;
    fixture.words[1] = UINT16_C(0x1000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
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

        /* Only SP, the program counter, and the staged RTN may move. */
        expect_arch = before_arch;
        expect_arch.pc_word = UINT32_C(0x1000);
        expect_arch.sp = UINT16_C(0x02FF);
        /* The delayed push stages the PC+4 return address into RTN. */
        expect_arch.rtn = UINT16_C(0x2116);
        expect_arch.scalar_known_mask = (uint8_t)(
            expect_arch.scalar_known_mask |
            (uint8_t)TI_C54X_ARCH_STATE_KNOWN_RTN);
        TI_C54X_TEST_CHECK(ti_c54x_test_arch_state_equal(&cpu->arch_state, &expect_arch));
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    test_worked_example();
    test_origins_and_targets();
    test_one_two_word_slot();
    test_inner_branch_wins();
    test_oversize_slot_still_branches();
    test_wait_resumes_armed();
    test_budget_split_drains();
    test_reset_clears_countdown();
    test_program_fault_in_slot();
    test_unknown_sp_rejects();
    test_data_fault_is_transactional();
    test_second_word_fault_is_transactional();
    test_architectural_preservation();
    puts("cpu_calld: ok");
    return 0;
}
