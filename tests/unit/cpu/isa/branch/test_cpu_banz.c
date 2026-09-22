/* Module contract
 * Owns: public-CPU RED proofs for the two-word `BANZ pmad,Sind`
 * auxiliary-register branch, including taken immediate commit of
 * the verbatim pmad with value-after post-decrement writeback,
 * not-taken sequential retirement with the side effect still
 * applied, observation-only retirement without AR mutation, exact
 * two-word fetch accounting, unknown-AR rejection without state
 * change, and full architectural preservation outside PC and the
 * selected AR.
 * Does not own: production CPU behavior, catalog/decode or pure
 * branch effects, address planning, shared fixtures, timing,
 * diagnostics, or test registration outside the branch leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"
#include "../../../../support/test_arch_state_compare.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct banz_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
} banz_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    banz_fixture *fixture = (banz_fixture *)context;
    uint32_t offset;

    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address >= fixture->base);
    offset = address - fixture->base;
    TI_C54X_TEST_CHECK(offset < fixture->word_count);
    *value = fixture->words[offset];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static ti_c54x_cpu *
init_cpu(banz_fixture *fixture, unsigned char *storage,
         uint32_t reset_pc)
{
    /* No DATA endpoint: BANZ retires with PC commit and AR writeback
     * only, so any endpoint traffic would fault here by
     * construction. */
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* FORM-T2-12-03: 0110_11Z0_SSSSSSSS with Z=0; word1 is the copied
 * raw pmad. Sind values below use I=1 with MOD0 (*ARx) or MOD1
 * (*ARx-): Sind = 1000_0MMM_XXX. */
static uint16_t
banz_word0(uint8_t sind)
{
    return (uint16_t)(UINT16_C(0x6C00) | sind);
}

static uint8_t
sind_for(bool decrement, uint8_t arx)
{
    return (uint8_t)(UINT8_C(0x80) |
                     (uint8_t)((decrement ? UINT8_C(1) : UINT8_C(0))
                               << 3U) |
                     arx);
}

static void
seed_cpu(ti_c54x_cpu *cpu, const uint16_t ars[8])
{
    uint8_t index;

    cpu->arch_state.status.st0 = UINT16_C(0x0000);
    cpu->arch_state.status.st1 = UINT16_C(0x0000);
    cpu->arch_state.status.pmst = UINT16_C(0x0000);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x1111222233)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000AABBCC)) ==
                       TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
    for (index = 0U; index < UINT8_C(8); ++index) {
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(
                               &cpu->arch_state, index, ars[index]) ==
                           TI_C54X_ARCH_STATE_OK);
    }
}

static banz_fixture
make_fixture(uint32_t base, uint8_t sind, uint16_t pmad)
{
    banz_fixture fixture = {0};

    fixture.base = base;
    fixture.words[0] = banz_word0(sind);
    fixture.words[1] = pmad;
    fixture.word_count = 2U;
    return fixture;
}

static void
expect_arch_preserved(const ti_c54x_cpu *cpu,
                      const ti_c54x_arch_state *before, uint32_t pc_word,
                      uint8_t arx, uint16_t ar_value)
{
    ti_c54x_arch_state expect_arch = *before;

    expect_arch.pc_word = pc_word;
    expect_arch.ar[arx] = ar_value;
    TI_C54X_TEST_CHECK(ti_c54x_test_arch_state_equal(&cpu->arch_state, &expect_arch));
}

static void
check_taken_case(uint32_t start_pc, uint8_t arx, bool decrement,
                 uint16_t ar_before, uint16_t pmad)
{
    banz_fixture fixture =
        make_fixture(start_pc, sind_for(decrement, arx), pmad);
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_state state;
    const uint16_t ars[8] = {UINT16_C(0x1111), UINT16_C(0x2222),
                             UINT16_C(0x3333), UINT16_C(0x4444),
                             UINT16_C(0x5555), UINT16_C(0x6666),
                             UINT16_C(0x7777), UINT16_C(0x8888)};
    const uint16_t value_after =
        (uint16_t)(decrement ? ar_before - UINT16_C(1) : ar_before);
    ti_c54x_arch_state saved_arch;
    ti_c54x_accumulator_state saved_accumulators;
    ti_c54x_t_register_state saved_t;

    cpu = init_cpu(&fixture, storage, start_pc);
    {
        uint16_t seeded[8];
        uint8_t index;

        for (index = 0U; index < UINT8_C(8); ++index) {
            seeded[index] = ars[index];
        }
        seeded[arx] = ar_before;
        seed_cpu(cpu, seeded);
    }
    saved_arch = cpu->arch_state;
    saved_accumulators = cpu->accumulators;
    saved_t = cpu->t_register;

    /* Taken BANZ commits the verbatim pmad immediately: a
     * non-delayed branch with no countdown. */
    TI_C54X_TEST_CHECK(ar_before != UINT16_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == banz_word0(
                                                  sind_for(decrement,
                                                           arx)));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(result.pc_word == (uint32_t)pmad);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)pmad);
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &saved_accumulators,
                              sizeof(saved_accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &saved_t,
                              sizeof(saved_t)) == 0);
    expect_arch_preserved(cpu, &saved_arch, (uint32_t)pmad, arx,
                          value_after);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == (uint32_t)pmad);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_not_taken_case(uint32_t start_pc, uint8_t arx, uint16_t pmad)
{
    banz_fixture fixture =
        make_fixture(start_pc, sind_for(true, arx), pmad);
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    const uint16_t ars[8] = {UINT16_C(0x1111), UINT16_C(0x2222),
                             UINT16_C(0x3333), UINT16_C(0x4444),
                             UINT16_C(0x5555), UINT16_C(0x6666),
                             UINT16_C(0x7777), UINT16_C(0x8888)};
    ti_c54x_arch_state saved_arch;
    ti_c54x_accumulator_state saved_accumulators;

    cpu = init_cpu(&fixture, storage, start_pc);
    {
        uint16_t seeded[8];
        uint8_t index;

        for (index = 0U; index < UINT8_C(8); ++index) {
            seeded[index] = ars[index];
        }
        seeded[arx] = UINT16_C(0);
        seed_cpu(cpu, seeded);
    }
    saved_arch = cpu->arch_state;
    saved_accumulators = cpu->accumulators;

    /* Not-taken BANZ retires sequentially, but the post-decrement
     * writeback still applies: AR wraps to 0xFFFF. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word ==
                       banz_word0(sind_for(true, arx)));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(result.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &saved_accumulators,
                              sizeof(saved_accumulators)) == 0);
    expect_arch_preserved(cpu, &saved_arch, start_pc + UINT32_C(2), arx,
                          UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_taken_commits_verbatim_target(void)
{
    /* Image-evidenced *AR4- loop shape: AR4=2 takes to 0x2000. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(4), true, UINT16_C(0x0002),
                     UINT16_C(0x2000));
    /* Final iteration still taken: AR4=1 takes, writes back 0. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(4), true, UINT16_C(0x0001),
                     UINT16_C(0x2000));
    /* Observation-only *AR4: taken with no AR mutation. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(4), false, UINT16_C(0x0001),
                     UINT16_C(0x2000));
    /* Neighbor AR3 with a distinct target. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(3), true, UINT16_C(0x00FF),
                     UINT16_C(0x1000));
    /* Degenerate target equal to the sequential successor still
     * commits immediately with no countdown. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(4), true, UINT16_C(0x0002),
                     UINT16_C(0x1002));
}

static void
test_not_taken_falls_through(void)
{
    /* AR4 exhausted: falls through, AR4 wraps to 0xFFFF. */
    check_not_taken_case(UINT32_C(0x1000), UINT8_C(4), UINT16_C(0x2000));
    /* Neighbor AR2 exhausted with a distinct target. */
    check_not_taken_case(UINT32_C(0x1000), UINT8_C(2), UINT16_C(0x1000));
}

static void
test_unknown_ar_rejects(void)
{
    banz_fixture fixture =
        make_fixture(UINT32_C(0x1000), sind_for(true, UINT8_C(4)),
                     UINT16_C(0x2000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    const uint16_t ars[8] = {UINT16_C(0x1111), UINT16_C(0x2222),
                             UINT16_C(0x3333), UINT16_C(0x4444),
                             UINT16_C(0x5555), UINT16_C(0x6666),
                             UINT16_C(0x7777), UINT16_C(0x8888)};

    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, ars);
    cpu->arch_state.ar_known_mask = (uint8_t)(
        cpu->arch_state.ar_known_mask & (uint8_t)~(UINT8_C(1) << 4U));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(
            !ti_c54x_sequencer_delayed_branch_is_armed(
                &cpu->delayed_branch));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_cmpt1_rejects(void)
{
    banz_fixture fixture =
        make_fixture(UINT32_C(0x1000), sind_for(true, UINT8_C(4)),
                     UINT16_C(0x2000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    const uint16_t ars[8] = {UINT16_C(0x1111), UINT16_C(0x2222),
                             UINT16_C(0x3333), UINT16_C(0x4444),
                             UINT16_C(0x5555), UINT16_C(0x6666),
                             UINT16_C(0x7777), UINT16_C(0x8888)};

    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, ars);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state,
                                                     UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        /* CMPT=1 leaves the repo operating condition: the
         * auxiliary-branch route rejects without retirement, AR
         * writeback, or PC movement. */
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(
            !ti_c54x_sequencer_delayed_branch_is_armed(
                &cpu->delayed_branch));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_taken_commits_verbatim_target();
    test_not_taken_falls_through();
    test_unknown_ar_rejects();
    test_cmpt1_rejects();
    return 0;
}
