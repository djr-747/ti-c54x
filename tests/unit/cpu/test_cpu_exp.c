/* Module contract
 * Owns: CPU-level EXP retirement proofs for private T state, source
 * preservation, candidate-PC ordering, and unknown-input rollback.
 * Does not own: EXP arithmetic, catalog/decode metadata, effect routing,
 * timing, repeatability, interrupts, memory, or the public T inspection API.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <ti_c54x/cpu.h>

#include "../../../src/cpu/cpu_internal.h"
#include "../../../src/state/accumulator/accumulator.h"
#include "../../../src/state/temporary/t_register.h"
#include "../../support/test_harness.h"

typedef struct exp_fixture {
    uint16_t word;
    ti_c54x_program_word_address expected_address;
    unsigned reads;
} exp_fixture;

static ti_c54x_bus_status read_word(void *context,
                                    ti_c54x_program_word_address address,
                                    uint16_t *value)
{
    exp_fixture *fixture = (exp_fixture *)context;

    fixture->reads++;
    TI_C54X_TEST_CHECK(address == fixture->expected_address);
    *value = fixture->word;
    return TI_C54X_BUS_OK;
}

static void seed_t(ti_c54x_cpu *cpu, uint16_t value, bool known)
{
    /* This is intentionally private.  Public state inspection must not grow a
     * T field merely to make the CPU proof convenient. */
    cpu->t_register.raw_value = value;
    cpu->t_register.known = known;
}

static void init_cpu(exp_fixture *fixture,
                     ti_c54x_program_word_address reset_pc_word,
                     unsigned char *storage,
                     ti_c54x_cpu **cpu)
{
    const ti_c54x_bus bus = {
        .context = fixture,
        .program_read_word = read_word,
    };
    const ti_c54x_config config = {
        .reset_pc_word = reset_pc_word,
        .trace = {0U, NULL, NULL},
    };

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, 512U, &config, &bus, cpu) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(*cpu != NULL);
}

static void test_t_is_zero_and_unknown_after_init_and_reset(void)
{
    exp_fixture fixture = {UINT16_C(0xF48E), 0U, 0U};
    _Alignas(max_align_t) unsigned char storage[512];
    ti_c54x_cpu *cpu = NULL;

    init_cpu(&fixture, 0U, storage, &cpu);
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(!cpu->t_register.known);

    seed_t(cpu, UINT16_C(0xA55A), true);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(!cpu->t_register.known);
    ti_c54x_cpu_deinit(cpu);
}

static void run_success_case(uint16_t word,
                             uint64_t accumulator_a,
                             uint64_t accumulator_b,
                             uint16_t expected_t)
{
    exp_fixture fixture = {word, 0U, 0U};
    _Alignas(max_align_t) unsigned char storage[512];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;

    init_cpu(&fixture, 0U, storage, &cpu);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           accumulator_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           accumulator_b) == TI_C54X_ACCUMULATOR_OK);
    seed_t(cpu, UINT16_C(0xA55A), true);
    before_status = cpu->arch_state.status;
    before_accumulators = cpu->accumulators;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == word);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(fixture.reads == 1U);

    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == expected_t);
    TI_C54X_TEST_CHECK(cpu->t_register.known);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == UINT64_C(1));
    TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
    TI_C54X_TEST_CHECK_U64(state.accumulator_a_raw40, accumulator_a);
    TI_C54X_TEST_CHECK_U64(state.accumulator_b_raw40, accumulator_b);
    TI_C54X_TEST_CHECK(state.accumulator_a_known);
    TI_C54X_TEST_CHECK(state.accumulator_b_known);
    ti_c54x_cpu_deinit(cpu);
}

static void run_unknown_selected_case(uint16_t word,
                                      bool selected_a,
                                      uint64_t unselected_value)
{
    exp_fixture fixture = {word, 0U, 0U};
    _Alignas(max_align_t) unsigned char storage[512];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state before_state;
    ti_c54x_state after_state;
    ti_c54x_run_result result;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;

    init_cpu(&fixture, 0U, storage, &cpu);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           selected_a ? TI_C54X_ACCUMULATOR_B
                                      : TI_C54X_ACCUMULATOR_A,
                           unselected_value) == TI_C54X_ACCUMULATOR_OK);
    seed_t(cpu, UINT16_C(0xA55A), true);
    before_status = cpu->arch_state.status;
    before_accumulators = cpu->accumulators;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &before_state) ==
                       TI_C54X_ERROR_OK);

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == word);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(fixture.reads == 1U);
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0xA55A));
    TI_C54X_TEST_CHECK(cpu->t_register.known);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &after_state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(after_state.pc_word == before_state.pc_word);
    TI_C54X_TEST_CHECK(after_state.last_fetched_word == word);
    TI_C54X_TEST_CHECK(after_state.last_fetch_valid);
    TI_C54X_TEST_CHECK(after_state.instructions_attempted_total ==
                       before_state.instructions_attempted_total + UINT64_C(1));
    TI_C54X_TEST_CHECK(after_state.instructions_retired_total ==
                       before_state.instructions_retired_total);
    TI_C54X_TEST_CHECK_U64(after_state.accumulator_a_raw40,
                           before_state.accumulator_a_raw40);
    TI_C54X_TEST_CHECK_U64(after_state.accumulator_b_raw40,
                           before_state.accumulator_b_raw40);
    TI_C54X_TEST_CHECK(after_state.accumulator_a_known ==
                       before_state.accumulator_a_known);
    TI_C54X_TEST_CHECK(after_state.accumulator_b_known ==
                       before_state.accumulator_b_known);
    ti_c54x_cpu_deinit(cpu);
}

static void test_t_write_waits_for_candidate_pc_validation(void)
{
    exp_fixture fixture = {UINT16_C(0xF48E), UINT32_C(0xFFFF), 0U};
    _Alignas(max_align_t) unsigned char storage[512];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;

    init_cpu(&fixture, UINT32_C(0xFFFF), storage, &cpu);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(1)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(2)) == TI_C54X_ACCUMULATOR_OK);
    seed_t(cpu, UINT16_C(0xA55A), true);
    before_status = cpu->arch_state.status;
    before_accumulators = cpu->accumulators;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason ==
                       TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0xF48E));
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0xFFFF));
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0xA55A));
    TI_C54X_TEST_CHECK(cpu->t_register.known);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0xFFFF));
    TI_C54X_TEST_CHECK(state.last_fetched_word == UINT16_C(0xF48E));
    TI_C54X_TEST_CHECK(state.last_fetch_valid);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == UINT64_C(1));
    TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(0));
    ti_c54x_cpu_deinit(cpu);
}

int main(void)
{
    test_t_is_zero_and_unknown_after_init_and_reset();

    /* A=1 -> +30 (0x001e), B=2 -> +29 (0x001d).  Running both forms proves
     * the S bit chooses the source while the opposite accumulator is copied
     * through the transaction untouched. */
    run_success_case(UINT16_C(0xF48E), UINT64_C(1), UINT64_C(2),
                     UINT16_C(0x001E));
    run_success_case(UINT16_C(0xF58E), UINT64_C(1), UINT64_C(2),
                     UINT16_C(0x001D));

    run_unknown_selected_case(UINT16_C(0xF48E), true, UINT64_C(2));
    run_unknown_selected_case(UINT16_C(0xF58E), false, UINT64_C(1));
    test_t_write_waits_for_candidate_pc_validation();
    return 0;
}
