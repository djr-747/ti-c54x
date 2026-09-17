/* Module contract
 * Owns: public-CPU RED proofs for one-word `XC n,cond` retirement,
 * including taken sequential execution and not-taken NOP skips
 * for n = 1 and 2 with SSBX-follower effect visibility, program
 * accounting with no skipped-word fetch traffic, T and
 * architectural preservation, deferred-condition transactional
 * rejection, and the TI Example taken lane.
 * Does not own: catalog/decode or pure skip effects, shared
 * fixtures, endpoint behavior, timing, diagnostics, or test
 * registration outside the branch leaf.
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

typedef struct xc_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
} xc_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    xc_fixture *fixture = (xc_fixture *)context;
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
init_cpu(xc_fixture *fixture, unsigned char *storage, uint32_t reset_pc)
{
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* FORM-T2-18-07: 1111_11N1_CCCCCCCC with N at bit 9 and the
 * condition field in the low byte. */
static uint16_t
xc_word0(uint8_t select, uint8_t cond)
{
    return (uint16_t)(UINT16_C(0xFD00) | ((uint16_t)(select & UINT8_C(1))
                                          << 9U) |
                      cond);
}

/* SSBX ST1 bit 0: visible one-word follower with no bus traffic. */
static uint16_t
ssbx_st1_bit0(void)
{
    return UINT16_C(0xF7B0);
}

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
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           acc_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           acc_b) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(
                           &cpu->arch_state.status, c) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(
                           &cpu->arch_state.status, tc) ==
                       TI_C54X_STATUS_REGISTER_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
}

static void
check_taken_n1_executes_follower(void)
{
    /* TI SPRU172C §XC Example shape: XC 1,ALEQ with A holding -1
     * takes, so the SSBX follower executes. */
    xc_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_arch_state saved_arch;
    ti_c54x_accumulator_state saved_accumulators;

    fixture.base = UINT32_C(0x0100);
    fixture.words[0] = xc_word0(0, UINT8_C(0x47));
    fixture.words[1] = ssbx_st1_bit0();
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, fixture.base);
    seed_cpu(cpu, UINT64_C(0xFFFFFFFFFF), UINT64_C(0), UINT8_C(0),
             UINT8_C(0));
    saved_arch = cpu->arch_state;
    saved_accumulators = cpu->accumulators;

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == fixture.base + UINT32_C(2));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 & UINT16_C(1)) !=
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &saved_accumulators,
                              sizeof(saved_accumulators)) == 0);
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x2468));
    {
        ti_c54x_arch_state expect_arch = saved_arch;

        expect_arch.pc_word = fixture.base + UINT32_C(2);
        expect_arch.status.st1 = (uint16_t)(saved_arch.status.st1 | 1U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &expect_arch,
                                  sizeof(expect_arch)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_not_taken_n1_skips_follower(void)
{
    /* XC 1,AGT with A holding -1 misses: the SSBX follower is never
     * fetched and the trailing NOP executes. */
    xc_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x0200);
    fixture.words[0] = xc_word0(0, UINT8_C(0x46));
    fixture.words[1] = ssbx_st1_bit0();
    fixture.words[2] = UINT16_C(0xF495);
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, fixture.base);
    seed_cpu(cpu, UINT64_C(0xFFFFFFFFFF), UINT64_C(0), UINT8_C(0),
             UINT8_C(0));

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == fixture.base + UINT32_C(3));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 & UINT16_C(1)) ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x2468));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_taken_n2_executes_pair(void)
{
    xc_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x0300);
    fixture.words[0] = xc_word0(1, UINT8_C(0x30));
    fixture.words[1] = UINT16_C(0xF495);
    fixture.words[2] = ssbx_st1_bit0();
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, fixture.base);
    seed_cpu(cpu, UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1));

    result = ti_c54x_cpu_run(cpu, UINT64_C(3));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(3));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(3));
    TI_C54X_TEST_CHECK(fixture.program_calls == 3U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == fixture.base + UINT32_C(3));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 & UINT16_C(1)) !=
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_not_taken_n2_skips_pair(void)
{
    /* XC 2,NTC with TC set misses: both followers are skipped and
     * the trailing NOP executes. */
    xc_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x0400);
    fixture.words[0] = xc_word0(1, UINT8_C(0x20));
    fixture.words[1] = UINT16_C(0xF495);
    fixture.words[2] = ssbx_st1_bit0();
    fixture.words[3] = UINT16_C(0xF495);
    fixture.word_count = 4U;
    cpu = init_cpu(&fixture, storage, fixture.base);
    seed_cpu(cpu, UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1));

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == fixture.base + UINT32_C(4));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 & UINT16_C(1)) ==
                       UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
check_deferred_condition_stops(void)
{
    xc_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_arch_state saved_arch;

    fixture.base = UINT32_C(0x0500);
    fixture.words[0] = xc_word0(0, UINT8_C(0x03));
    fixture.words[1] = UINT16_C(0xF495);
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, fixture.base);
    seed_cpu(cpu, UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(0));
    saved_arch = cpu->arch_state;

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == fixture.base);
    TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &saved_arch,
                              sizeof(saved_arch)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    check_taken_n1_executes_follower();
    check_not_taken_n1_skips_follower();
    check_taken_n2_executes_pair();
    check_not_taken_n2_skips_pair();
    check_deferred_condition_stops();
    return 0;
}
