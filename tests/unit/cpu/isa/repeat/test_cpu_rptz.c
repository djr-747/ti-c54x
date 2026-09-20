/* Module contract
 * Owns: public-CPU RED proofs for `RPTZ dst,#lk` activation,
 * including the same-retirement dst zero, RC load, body
 * repetition lk+1 times, and status preservation. Does not own:
 * production CPU behavior, catalog/decode or pure repeat
 * effects, the single-repeat driver, shared fixtures, timing,
 * diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/repeat_counter_register.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t NOP_WORD = UINT16_C(0xF495);

typedef struct rptz_fixture {
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
} rptz_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    rptz_fixture *fixture = (rptz_fixture *)context;
    uint32_t offset;

    /* HOLD re-drives body fetches at a repeated address, so
     * serve by address rather than asserting call order. */
    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address < fixture->word_count);
    offset = address;
    *value = fixture->words[offset];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static ti_c54x_cpu *
init_cpu(rptz_fixture *fixture, unsigned char *storage)
{
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

static void
test_rptz_zeroes_dst_and_repeats(void)
{
    /* RPTZ A,#2 with A and B seeded nonzero and C/TC/OV set:
     * A zeroes in the same retirement that loads RC=2, the NOP
     * body runs 3 times, B is untouched, and no status bit
     * moves (TI pledges none). */
    rptz_fixture fixture = {.words = {UINT16_C(0xF071), UINT16_C(2),
                                      NOP_WORD, NOP_WORD},
                            .word_count = 4U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
    ti_c54x_run_result result;
    ti_c54x_state state = {0};
    uint16_t rc = UINT16_C(0xBEEF);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(&cpu->accumulators,
                                                TI_C54X_ACCUMULATOR_A,
                                                UINT64_C(0x123456789A)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(&cpu->accumulators,
                                                TI_C54X_ACCUMULATOR_B,
                                                UINT64_C(0x0FEDCBA987)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(
                           &cpu->arch_state.status, UINT8_C(1)) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(
                           &cpu->arch_state.status, UINT8_C(1)) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_ova(
                           &cpu->arch_state.status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rc(&cpu->arch_state, &rc) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(rc == UINT16_C(2));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE);
    TI_C54X_TEST_CHECK(cpu->repeat_progress.body_executions_remaining ==
                       UINT32_C(3));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0));
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == UINT64_C(0x0FEDCBA987));
    TI_C54X_TEST_CHECK(state.accumulator_a_known);
    TI_C54X_TEST_CHECK(state.c == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_tc(
                           &cpu->arch_state.status)
                           .raw_value == UINT16_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_ova(
                           &cpu->arch_state.status)
                           .raw_value == UINT16_C(1));

    /* The body runs exactly 3 times, then execution advances
     * past it: 3 body retirements plus the next NOP. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(4));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(4));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(4));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_rptz_zeroes_b(void)
{
    /* D=1 selects B; lk=0 repeats the body once. */
    rptz_fixture fixture = {.words = {UINT16_C(0xF171), UINT16_C(0),
                                      NOP_WORD},
                            .word_count = 3U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
    ti_c54x_run_result result;
    ti_c54x_state state = {0};

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(&cpu->accumulators,
                                                TI_C54X_ACCUMULATOR_A,
                                                UINT64_C(0x123456789A)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(&cpu->accumulators,
                                                TI_C54X_ACCUMULATOR_B,
                                                UINT64_C(0x0FEDCBA987)) ==
                       TI_C54X_ACCUMULATOR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.body_executions_remaining ==
                       UINT32_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_b_raw40 == UINT64_C(0));
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x123456789A));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(3));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_rptz_zeroes_dst_and_repeats();
    test_rptz_zeroes_b();
    return 0;
}
