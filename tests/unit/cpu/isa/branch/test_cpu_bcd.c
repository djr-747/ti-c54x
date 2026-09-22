/* Module contract
 * Owns: public-CPU RED proofs for the two-word `BCD pmad,cond`
 * delayed conditional branch, including taken deferral with the
 * two-slot countdown and not-taken sequential retirement from
 * nonzero origins, the worked-example vectors, two-word slot
 * fills, inner-branch takeover, slot-fetch faulting, unknown-input
 * and second-word fault transactionality, and full architectural
 * preservation without endpoint traffic.
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

typedef struct bcd_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
    bool program_fault;
    uint32_t fault_address;
} bcd_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    bcd_fixture *fixture = (bcd_fixture *)context;
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
init_cpu(bcd_fixture *fixture, unsigned char *storage,
         uint32_t reset_pc)
{
    /* No DATA endpoint: BCD retires with PC advance and countdown
     * arming only, so any endpoint traffic would fault here by
     * construction. */
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* FORM-T2-12-04: 1111_10Z0_CCCCCCCC with Z=1; word1 is the copied
 * raw pmad. */
static uint16_t
bcd_word0(uint8_t cond)
{
    return (uint16_t)(UINT16_C(0xFA00) | cond);
}

static const uint16_t NOP_WORD0 = UINT16_C(0xF495);
/* LD #lk,A: 0xF062 with destination bit 8 clear selects A. */
static const uint16_t LD_LK_A_WORD0 = UINT16_C(0xF062);
/* B pmad: 0xF073 followed by the 16-bit target word. */
static const uint16_t B_WORD0 = UINT16_C(0xF073);

static void
seed_cpu(ti_c54x_cpu *cpu, uint64_t acc_a, uint64_t acc_b, uint8_t c,
         uint8_t tc)
{
    cpu->arch_state.status.st0 = UINT16_C(0x0000);
    cpu->arch_state.status.st1 = UINT16_C(0x0000);
    cpu->arch_state.status.pmst = UINT16_C(0x0000);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(
                           &cpu->arch_state.status, c) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(
                           &cpu->arch_state.status, tc) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state, 0U) ==
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

static bcd_fixture
make_fixture(uint32_t base)
{
    bcd_fixture fixture = {0};

    fixture.base = base;
    return fixture;
}

static void
check_taken_case(uint32_t start_pc, uint8_t cond, uint16_t pmad,
                 uint64_t acc_a, uint64_t acc_b, uint8_t c, uint8_t tc)
{
    bcd_fixture fixture = make_fixture(start_pc);
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_state state;

    fixture.words[0] = bcd_word0(cond);
    fixture.words[1] = pmad;
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_cpu(cpu, acc_a, acc_b, c, tc);

    /* Taken BCD retires first: PC advances sequentially and the
     * countdown arms with pmad deferred. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == bcd_word0(cond));
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
check_not_taken_case(uint32_t start_pc, uint8_t cond, uint16_t pmad,
                     uint64_t acc_a, uint64_t acc_b, uint8_t c,
                     uint8_t tc)
{
    bcd_fixture fixture = make_fixture(start_pc);
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = bcd_word0(cond);
    fixture.words[1] = pmad;
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_cpu(cpu, acc_a, acc_b, c, tc);

    /* Not-taken BCD retires sequentially with no countdown. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == bcd_word0(cond));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(result.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_taken_defers_target(void)
{
    /* TC taken to an arbitrary target from a nonzero origin. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(0x30), UINT16_C(0x2000),
                     UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1));
    /* C taken. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(0x0C), UINT16_C(0x3000),
                     UINT64_C(0), UINT64_C(0), UINT8_C(1), UINT8_C(0));
    /* TI BC 2000h,AGT shape: A=0053 takes. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(0x46), UINT16_C(0x2000),
                     UINT64_C(0x53), UINT64_C(0), UINT8_C(0),
                     UINT8_C(0));
    /* Degenerate target equal to the sequential successor still
     * defers through the countdown. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(0x30), UINT16_C(0x1002),
                     UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1));
}

static void
test_not_taken_falls_through(void)
{
    /* NTC falls through to PC+2. */
    check_not_taken_case(UINT32_C(0x1000), UINT8_C(0x20),
                         UINT16_C(0x2000), UINT64_C(0), UINT64_C(0),
                         UINT8_C(0), UINT8_C(1));
    /* NC falls through. */
    check_not_taken_case(UINT32_C(0x1000), UINT8_C(0x08),
                         UINT16_C(0x3000), UINT64_C(0), UINT64_C(0),
                         UINT8_C(1), UINT8_C(0));
    /* TI BC 2000h,AGT shape: negative A does not take. */
    check_not_taken_case(UINT32_C(0x1000), UINT8_C(0x46),
                         UINT16_C(0x2000), UINT64_C(0xFFFFFFFFFF),
                         UINT64_C(0), UINT8_C(0), UINT8_C(0));
}

static void
test_taken_two_word_slot(void)
{
    bcd_fixture fixture = make_fixture(UINT32_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_accumulator_snapshot after_a;

    /* One 2-word LD fills both delay slots and must really execute. */
    fixture.words[0] = bcd_word0(UINT8_C(0x30));
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = LD_LK_A_WORD0;
    fixture.words[3] = UINT16_C(0x1234);
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1));
    seed_sxm(cpu, UINT8_C(0));

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2000));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_read_snapshot(
                           &cpu->accumulators, &after_a) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(after_a.a_raw40 == UINT64_C(0x12340000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_inner_branch_wins(void)
{
    bcd_fixture fixture = make_fixture(UINT32_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    /* A branch inside the delay slots is TI-forbidden; the inner
     * redirect deterministically cancels the pending countdown. */
    fixture.words[0] = bcd_word0(UINT8_C(0x30));
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = B_WORD0;
    fixture.words[3] = UINT16_C(0x3000);
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1));

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
test_program_fault_in_slot(void)
{
    bcd_fixture fixture = make_fixture(UINT32_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = bcd_word0(UINT8_C(0x30));
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    fixture.program_fault = true;
    fixture.fault_address = UINT32_C(0x1002);
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1));

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* The faulting slot never retired, so the countdown holds. */
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1002));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_input_rejects(void)
{
    bcd_fixture fixture = make_fixture(UINT32_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = bcd_word0(UINT8_C(0x30));
    fixture.words[1] = UINT16_C(0x2000);
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1));
    cpu->arch_state.status.st0_known_mask = (uint16_t)(
        cpu->arch_state.status.st0_known_mask &
        (uint16_t)~TI_C54X_ST0_TC_MASK);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
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
    bcd_fixture fixture = make_fixture(UINT32_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = bcd_word0(UINT8_C(0x30));
    fixture.words[1] = UINT16_C(0x2000);
    fixture.word_count = 2U;
    fixture.program_fault = true;
    fixture.fault_address = UINT32_C(0x1001);
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1));
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
    bcd_fixture fixture = make_fixture(UINT32_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.words[0] = bcd_word0(UINT8_C(0x30));
    fixture.words[1] = UINT16_C(0x2000);
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT64_C(0x1111222233), UINT64_C(0xAABBCC),
             UINT8_C(1), UINT8_C(1));
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
    test_taken_defers_target();
    test_not_taken_falls_through();
    test_taken_two_word_slot();
    test_inner_branch_wins();
    test_program_fault_in_slot();
    test_unknown_input_rejects();
    test_second_word_fault_is_transactional();
    test_architectural_preservation();
    puts("cpu_bcd: ok");
    return 0;
}
