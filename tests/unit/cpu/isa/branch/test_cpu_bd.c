/* Module contract
 * Owns: public-CPU RED proofs for the two-word `BD`
 * delayed unconditional branch, including verbatim pmad
 * deferral from nonzero origins, the two-slot countdown with
 * sequential fetch, deferred-branch take-effect, one- and
 * two-word slot fills, inner-branch takeover, inner-delayed
 * replacement, oversize-fill saturation, budget-split
 * persistence, reset clearing, slot-fetch faulting, second-word
 * fault transactionality, and full architectural preservation
 * outside PC without endpoint traffic.
 * Does not own: production CPU behavior, catalog/decode or pure
 * branch effects, address planning, shared fixtures, timing,
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

typedef struct bd_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
    bool program_fault;
    uint32_t fault_address;
} bd_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    bd_fixture *fixture = (bd_fixture *)context;
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

static ti_c54x_cpu *
init_cpu(bd_fixture *fixture, unsigned char *storage,
         uint32_t reset_pc)
{
    /* No DATA endpoint: BD retires with sequential PC advance and
     * countdown arming only, so any endpoint traffic would fault
     * here by construction. */
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* FORM-T2-12-01: 1111_00Z0_0011_0011 with Z=1; word1 is the copied
 * raw pmad. */
static const uint16_t BD_WORD0 = UINT16_C(0xF273);
static const uint16_t NOP_WORD0 = UINT16_C(0xF495);
/* LD #lk,A: 0xF062 with destination bit 8 clear selects A. */
static const uint16_t LD_LK_A_WORD0 = UINT16_C(0xF062);
/* B pmad: 0xF073 followed by the 16-bit target word. */
static const uint16_t B_WORD0 = UINT16_C(0xF073);

static void
seed_cpu(ti_c54x_cpu *cpu)
{
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
    cpu->arch_state.status.pmst = UINT16_C(0x3C3C);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state, 0U) ==
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

static bd_fixture
make_fixture(uint32_t base)
{
    bd_fixture fixture = {0};

    fixture.base = base;
    return fixture;
}

static void
check_ready_case(uint32_t start_pc, uint16_t pmad)
{
    bd_fixture fixture = make_fixture(start_pc);
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_state state;

    fixture.words[0] = BD_WORD0;
    fixture.words[1] = pmad;
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_cpu(cpu);

    /* BD retires first: PC advances sequentially and the countdown
     * arms with pmad deferred. Run counters are per call. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == BD_WORD0);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word == pmad);

    /* Both delay slots retire, then the deferred branch takes effect. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.pc_word == (uint32_t)pmad);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)pmad);
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == (uint32_t)pmad);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_ready_defers_verbatim_target(void)
{
    /* Degenerate pmad values exercise targets at and below the
     * sequential successor without taking it early. */
    static const uint16_t boundaries[] = {
        UINT16_C(0x0000), UINT16_C(0x0001), UINT16_C(0x0002),
        UINT16_C(0x0003), UINT16_C(0x00FF), UINT16_C(0x1F44),
        UINT16_C(0x1F45), UINT16_C(0x2000), UINT16_C(0x7FFF),
        UINT16_C(0x8000), UINT16_C(0xFEDC), UINT16_C(0xFFFE),
        UINT16_C(0xFFFF)};
    size_t index;

    for (index = 0U;
         index < sizeof(boundaries) / sizeof(boundaries[0]); ++index) {
        check_ready_case(UINT32_C(0), boundaries[index]);
    }
    /* Prime-stride sweep covers the remaining pmad space. */
    for (uint32_t raw = 0U; raw <= UINT32_C(0xFFFF); raw += 257U) {
        check_ready_case(UINT32_C(0), (uint16_t)raw);
    }
}

static void
test_worked_example_from_nonzero_origin(void)
{
    /* TI SPRU172C B 2000h example shape: PC 1F45 becomes PC 2000
     * after the two delay slots. */
    check_ready_case(UINT32_C(0x1F45), UINT16_C(0x2000));
}

static void
test_one_two_word_slot(void)
{
    bd_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_accumulator_snapshot before_b;
    ti_c54x_accumulator_snapshot after_a;
    ti_c54x_accumulator_snapshot after_b;

    /* One 2-word LD fills both delay slots and must really execute. */
    fixture.words[0] = BD_WORD0;
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = LD_LK_A_WORD0;
    fixture.words[3] = UINT16_C(0x1234);
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu);
    seed_sxm(cpu, UINT8_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_snapshot(
                           &cpu->accumulators, &before_b) ==
                       TI_C54X_ACCUMULATOR_OK);

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2000));
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
    bd_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    /* A branch inside the delay slots is TI-forbidden; the inner
     * redirect deterministically cancels the pending countdown. */
    fixture.words[0] = BD_WORD0;
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = B_WORD0;
    fixture.words[3] = UINT16_C(0x3000);
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu);

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x3000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_inner_delayed_branch_replaces(void)
{
    bd_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    /* A delayed branch inside the delay slots is TI-forbidden; the
     * inner arming deterministically replaces the pending
     * countdown under the last-wins rule. */
    fixture.words[0] = BD_WORD0;
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = BD_WORD0;
    fixture.words[3] = UINT16_C(0x3000);
    fixture.words[4] = NOP_WORD0;
    fixture.words[5] = NOP_WORD0;
    fixture.word_count = 6U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu);

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word ==
                       UINT16_C(0x3000));

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x3000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_oversize_slot_still_branches(void)
{
    bd_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    /* One word left with a two-word instruction retiring: the
     * malformed fill still takes the branch. */
    fixture.words[0] = BD_WORD0;
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = LD_LK_A_WORD0;
    fixture.words[4] = UINT16_C(0x1234);
    fixture.word_count = 5U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu);
    seed_sxm(cpu, UINT8_C(0));

    result = ti_c54x_cpu_run(cpu, UINT64_C(3));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(3));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(3));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_budget_split_drains(void)
{
    bd_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = BD_WORD0;
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu);

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
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_reset_clears_countdown(void)
{
    bd_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = BD_WORD0;
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu);

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
    bd_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = BD_WORD0;
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    fixture.program_fault = true;
    fixture.fault_address = UINT32_C(0x2114);
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu);

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* The faulting slot never retired, so the countdown holds. */
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2114));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_second_word_fault_is_transactional(void)
{
    bd_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = BD_WORD0;
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    fixture.program_fault = true;
    fixture.fault_address = UINT32_C(0x2113);
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu);
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
    bd_fixture fixture = make_fixture(UINT32_C(0x2112));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = BD_WORD0;
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    seed_cpu(cpu);
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

        /* Only the program counter may move. */
        expect_arch = before_arch;
        expect_arch.pc_word = UINT32_C(0x2000);
        TI_C54X_TEST_CHECK(ti_c54x_test_arch_state_equal(&cpu->arch_state, &expect_arch));
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    test_ready_defers_verbatim_target();
    test_worked_example_from_nonzero_origin();
    test_one_two_word_slot();
    test_inner_branch_wins();
    test_inner_delayed_branch_replaces();
    test_oversize_slot_still_branches();
    test_budget_split_drains();
    test_reset_clears_countdown();
    test_program_fault_in_slot();
    test_second_word_fault_is_transactional();
    test_architectural_preservation();
    puts("cpu_bd: ok");
    return 0;
}
