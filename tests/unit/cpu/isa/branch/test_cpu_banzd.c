/* Module contract
 * Owns: public-CPU RED proofs for the two-word `BANZD pmad,Sind`
 * delayed auxiliary-register branch, including taken deferral
 * with the two-slot countdown drain and verbatim pmad commit,
 * not-taken sequential retirement with the side effect still
 * applied, observation-only retirement without AR mutation,
 * exact fetch accounting, unknown-AR and CMPT=1 rejection
 * without state change, and full architectural preservation
 * outside PC and the selected AR.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct banzd_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
} banzd_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    banzd_fixture *fixture = (banzd_fixture *)context;
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
init_cpu(banzd_fixture *fixture, unsigned char *storage,
         uint32_t reset_pc)
{
    /* No DATA endpoint: BANZD retires with PC commit, countdown
     * arming, and AR writeback only, so any endpoint traffic would
     * fault here by construction. */
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* FORM-T2-12-03 delayed half: 0110_1110_SSSSSSSS; word1 is the
 * copied raw pmad. Sind values below use I=1 with MOD0 (*ARx) or
 * MOD1 (*ARx-): Sind = 1000_0MMM_XXX. */
static uint16_t
banzd_word0(uint8_t sind)
{
    return (uint16_t)(UINT16_C(0x6E00) | sind);
}

static uint8_t
sind_for(bool decrement, uint8_t arx)
{
    return (uint8_t)(UINT8_C(0x80) |
                     (uint8_t)((decrement ? UINT8_C(1) : UINT8_C(0))
                               << 3U) |
                     arx);
}

static const uint16_t NOP_WORD0 = UINT16_C(0xF495);

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

static void
seed_case_ars(uint16_t seeded[8], uint8_t arx, uint16_t ar_before)
{
    static const uint16_t sentinels[8] = {
        UINT16_C(0x1111), UINT16_C(0x2222), UINT16_C(0x3333),
        UINT16_C(0x4444), UINT16_C(0x5555), UINT16_C(0x6666),
        UINT16_C(0x7777), UINT16_C(0x8888)};
    uint8_t index;

    for (index = 0U; index < UINT8_C(8); ++index) {
        seeded[index] = sentinels[index];
    }
    seeded[arx] = ar_before;
}

static void
check_taken_case(uint32_t start_pc, uint8_t arx, bool decrement,
                 uint16_t ar_before, uint16_t pmad)
{
    banzd_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_state state;
    uint16_t seeded[8];
    const uint16_t value_after =
        (uint16_t)(decrement ? ar_before - UINT16_C(1) : ar_before);
    ti_c54x_arch_state saved_arch;
    ti_c54x_accumulator_state saved_accumulators;
    ti_c54x_t_register_state saved_t;

    fixture.base = start_pc;
    fixture.words[0] = banzd_word0(sind_for(decrement, arx));
    fixture.words[1] = pmad;
    fixture.words[2] = NOP_WORD0;
    fixture.words[3] = NOP_WORD0;
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_case_ars(seeded, arx, ar_before);
    seed_cpu(cpu, seeded);
    saved_arch = cpu->arch_state;
    saved_accumulators = cpu->accumulators;
    saved_t = cpu->t_register;

    /* Taken BANZD retires first: PC advances sequentially and the
     * countdown arms with pmad deferred. */
    TI_C54X_TEST_CHECK(ar_before != UINT16_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word ==
                       banzd_word0(sind_for(decrement, arx)));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word == pmad);
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &saved_accumulators,
                              sizeof(saved_accumulators)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &saved_t,
                              sizeof(saved_t)) == 0);
    {
        ti_c54x_arch_state expect_arch = saved_arch;

        expect_arch.pc_word = start_pc + UINT32_C(2);
        expect_arch.ar[arx] = value_after;
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &expect_arch,
                                  sizeof(expect_arch)) == 0);
    }

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
check_not_taken_case(uint32_t start_pc, uint8_t arx, uint16_t pmad)
{
    banzd_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    uint16_t seeded[8];
    ti_c54x_arch_state saved_arch;
    ti_c54x_accumulator_state saved_accumulators;

    fixture.base = start_pc;
    fixture.words[0] = banzd_word0(sind_for(true, arx));
    fixture.words[1] = pmad;
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_case_ars(seeded, arx, UINT16_C(0));
    seed_cpu(cpu, seeded);
    saved_arch = cpu->arch_state;
    saved_accumulators = cpu->accumulators;

    /* Not-taken BANZD retires sequentially with no countdown, but
     * the post-decrement writeback still applies: AR wraps. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word ==
                       banzd_word0(sind_for(true, arx)));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(result.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &saved_accumulators,
                              sizeof(saved_accumulators)) == 0);
    {
        ti_c54x_arch_state expect_arch = saved_arch;

        expect_arch.pc_word = start_pc + UINT32_C(2);
        expect_arch.ar[arx] = UINT16_C(0xFFFF);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &expect_arch,
                                  sizeof(expect_arch)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_taken_defers_target(void)
{
    /* Post-decrement loop shape takes through the countdown. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(4), true, UINT16_C(0x0002),
                     UINT16_C(0x2000));
    /* Final iteration still takes, writing back zero. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(4), true, UINT16_C(0x0001),
                     UINT16_C(0x2000));
    /* Observation-only shape takes with no AR mutation. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(4), false, UINT16_C(0x0001),
                     UINT16_C(0x2000));
    /* Neighbor AR3 with a distinct target. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(3), true, UINT16_C(0x00FF),
                     UINT16_C(0x1000));
    /* Degenerate target equal to the sequential successor still
     * defers through the countdown. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(4), true, UINT16_C(0x0002),
                     UINT16_C(0x1002));
}

static void
test_not_taken_falls_through(void)
{
    check_not_taken_case(UINT32_C(0x1000), UINT8_C(4), UINT16_C(0x2000));
    check_not_taken_case(UINT32_C(0x1000), UINT8_C(2), UINT16_C(0x1000));
}

static void
test_unknown_ar_rejects(void)
{
    banzd_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    uint16_t seeded[8];

    fixture.base = UINT32_C(0x1000);
    fixture.words[0] = banzd_word0(sind_for(true, UINT8_C(4)));
    fixture.words[1] = UINT16_C(0x2000);
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_case_ars(seeded, UINT8_C(4), UINT16_C(0x0002));
    seed_cpu(cpu, seeded);
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
    banzd_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    uint16_t seeded[8];

    fixture.base = UINT32_C(0x1000);
    fixture.words[0] = banzd_word0(sind_for(true, UINT8_C(4)));
    fixture.words[1] = UINT16_C(0x2000);
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_case_ars(seeded, UINT8_C(4), UINT16_C(0x0002));
    seed_cpu(cpu, seeded);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state,
                                                     UINT8_C(1)) ==
                       TI_C54X_ARCH_STATE_OK);
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

int
main(void)
{
    test_taken_defers_target();
    test_not_taken_falls_through();
    test_unknown_ar_rejects();
    test_cmpt1_rejects();
    return 0;
}
