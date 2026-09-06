/*
 * Module contract
 *
 * Owns: CPU instruction-budget, per-call accounting, stop-loop, and counter
 * overflow proofs. Private state access is intentional here so overflow can
 * be tested without relying on a wraparound-producing public setup.
 * Does not own: public CPU lifecycle policy, bridge implementation, opcode
 * semantics, timing, observer policy, diagnostics, or emulator integration.
 */

#include <stdint.h>
#include <stdlib.h>

#include <ti_c54x/cpu.h>

#include "../../../src/cpu/cpu_internal.h"
#include "../../support/test_harness.h"

typedef struct budget_bus_fixture {
    uint16_t words[4];
    ti_c54x_bus_status statuses[4];
    unsigned reads;
} budget_bus_fixture;

static ti_c54x_bus_status
read_budget(void *context,
            ti_c54x_program_word_address address_word,
            uint16_t *value_word)
{
    budget_bus_fixture *fixture = (budget_bus_fixture *)context;
    const unsigned index = fixture->reads;

    (void)address_word;
    TI_C54X_TEST_CHECK(index < 4U);
    fixture->reads++;
    if (fixture->statuses[index] != TI_C54X_BUS_OK) {
        return fixture->statuses[index];
    }
    *value_word = fixture->words[index];
    return TI_C54X_BUS_OK;
}

static ti_c54x_cpu *
make_cpu(void **storage_out, budget_bus_fixture *fixture)
{
    ti_c54x_bus bus = {0};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    bus.context = fixture;
    bus.program_read_word = read_budget;
    *storage_out = malloc(ti_c54x_cpu_size());
    TI_C54X_TEST_CHECK(*storage_out != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(*storage_out, ti_c54x_cpu_size(),
                                        &config, &bus, &cpu) ==
                       TI_C54X_ERROR_OK);
    return cpu;
}

static void
test_multi_nop_budget_and_cumulative_counts(void)
{
    budget_bus_fixture fixture = {{UINT16_C(0xf495), UINT16_C(0xf495),
                                   UINT16_C(0x1234), UINT16_C(0xf495)},
                                  {TI_C54X_BUS_OK, TI_C54X_BUS_OK,
                                   TI_C54X_BUS_OK, TI_C54X_BUS_OK},
                                  0U};
    void *storage = NULL;
    ti_c54x_cpu *cpu = make_cpu(&storage, &fixture);
    ti_c54x_run_result result;
    ti_c54x_state state;

    result = ti_c54x_cpu_run(cpu, 2U);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 2U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 2U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(fixture.reads == 2U);

    result = ti_c54x_cpu_run(cpu, 3U);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNIMPLEMENTED_OPCODE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(fixture.reads == 3U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 3U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 2U);

    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_step_uses_one_instruction_budget(void)
{
    budget_bus_fixture fixture = {{UINT16_C(0xf495), 0U, 0U, 0U},
                                  {TI_C54X_BUS_OK, TI_C54X_BUS_OK,
                                   TI_C54X_BUS_OK, TI_C54X_BUS_OK},
                                  0U};
    void *storage = NULL;
    ti_c54x_cpu *cpu = make_cpu(&storage, &fixture);
    ti_c54x_run_result result = ti_c54x_cpu_step(cpu);

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(fixture.reads == 1U);
    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_same_call_nop_then_unknown_stops_without_retry(void)
{
    budget_bus_fixture fixture = {{UINT16_C(0xf495), UINT16_C(0x1234), 0U, 0U},
                                  {TI_C54X_BUS_OK, TI_C54X_BUS_OK,
                                   TI_C54X_BUS_OK, TI_C54X_BUS_OK},
                                  0U};
    void *storage = NULL;
    ti_c54x_cpu *cpu = make_cpu(&storage, &fixture);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, 3U);
    ti_c54x_state state;

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNIMPLEMENTED_OPCODE);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 2U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(fixture.reads == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 2U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 1U);

    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_same_call_nop_then_bus_fault_stops_without_retry(void)
{
    budget_bus_fixture fixture = {{UINT16_C(0xf495), 0U, 0U, 0U},
                                  {TI_C54X_BUS_OK, TI_C54X_BUS_FAULT,
                                   TI_C54X_BUS_OK, TI_C54X_BUS_OK},
                                  0U};
    void *storage = NULL;
    ti_c54x_cpu *cpu = make_cpu(&storage, &fixture);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, 3U);
    ti_c54x_state state;

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 2U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(!result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.reads == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 2U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 1U);

    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_attempt_counter_overflow_stops_without_fetch_or_mutation(void)
{
    budget_bus_fixture fixture = {{UINT16_C(0xf495), 0U, 0U, 0U},
                                  {TI_C54X_BUS_OK, TI_C54X_BUS_OK,
                                   TI_C54X_BUS_OK, TI_C54X_BUS_OK},
                                  0U};
    void *storage = NULL;
    ti_c54x_cpu *cpu = make_cpu(&storage, &fixture);
    ti_c54x_run_result result;

    cpu->instructions_attempted = UINT64_MAX;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_COUNTER_OVERFLOW);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 0U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(!result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->instructions_attempted == UINT64_MAX);
    TI_C54X_TEST_CHECK(cpu->instructions_retired == 0U);
    TI_C54X_TEST_CHECK(cpu->pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(!cpu->last_fetch_valid);
    TI_C54X_TEST_CHECK(cpu->last_fetched_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(!cpu->execution_active);
    TI_C54X_TEST_CHECK(fixture.reads == 0U);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(result.opcode_valid);

    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_retired_counter_overflow_stops_without_fetch_or_mutation(void)
{
    budget_bus_fixture fixture = {{UINT16_C(0xf495), 0U, 0U, 0U},
                                  {TI_C54X_BUS_OK, TI_C54X_BUS_OK,
                                   TI_C54X_BUS_OK, TI_C54X_BUS_OK},
                                  0U};
    void *storage = NULL;
    ti_c54x_cpu *cpu = make_cpu(&storage, &fixture);
    ti_c54x_run_result result;

    cpu->instructions_retired = UINT64_MAX;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_COUNTER_OVERFLOW);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 0U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(!result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->instructions_attempted == 0U);
    TI_C54X_TEST_CHECK(cpu->instructions_retired == UINT64_MAX);
    TI_C54X_TEST_CHECK(cpu->pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(!cpu->last_fetch_valid);
    TI_C54X_TEST_CHECK(cpu->last_fetched_word == UINT16_C(0));
    TI_C54X_TEST_CHECK(!cpu->execution_active);
    TI_C54X_TEST_CHECK(fixture.reads == 0U);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(result.opcode_valid);

    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

int
main(void)
{
    test_multi_nop_budget_and_cumulative_counts();
    test_step_uses_one_instruction_budget();
    test_same_call_nop_then_unknown_stops_without_retry();
    test_same_call_nop_then_bus_fault_stops_without_retry();
    test_attempt_counter_overflow_stops_without_fetch_or_mutation();
    test_retired_counter_overflow_stops_without_fetch_or_mutation();
    return 0;
}
