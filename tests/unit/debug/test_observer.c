/* Module contract
 * Owns: executable tests for the optional structured observer boundary.
 * Does not own: observer implementation, CPU storage, bus memory, or output
 * policy. This target is built against the observer-enabled library variant.
 */

#include <stdint.h>
#include <stdlib.h>

#include <ti_c54x/cpu.h>

#include "../../support/test_harness.h"

typedef struct observer_fixture {
    uint16_t opcode_word;
    unsigned reads;
} observer_fixture;

typedef struct event_log {
    ti_c54x_trace_kind kinds[8];
    unsigned count;
    ti_c54x_cpu *cpu_for_reentry;
    ti_c54x_run_result nested_result;
    unsigned reentry_attempts;
} event_log;

static ti_c54x_bus_status
read_program(void *context,
             ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    observer_fixture *fixture = (observer_fixture *)context;

    (void)address_word;
    fixture->reads++;
    *value_word = fixture->opcode_word;
    return TI_C54X_BUS_OK;
}

static void
record_event(void *context, const ti_c54x_trace_event *event)
{
    event_log *log = (event_log *)context;

    TI_C54X_TEST_CHECK(event != NULL);
    TI_C54X_TEST_CHECK(log->count < sizeof(log->kinds) / sizeof(log->kinds[0]));
    log->kinds[log->count++] = event->kind;
    if (event->kind == TI_C54X_TRACE_FETCH && log->cpu_for_reentry != NULL) {
        log->reentry_attempts++;
        ti_c54x_cpu_deinit(log->cpu_for_reentry);
        log->nested_result = ti_c54x_cpu_step(log->cpu_for_reentry);
    }
}

static ti_c54x_bus
make_bus(observer_fixture *fixture)
{
    ti_c54x_bus bus = {0};

    bus.context = fixture;
    bus.program_read_word = read_program;
    return bus;
}

static ti_c54x_cpu *
make_cpu(void **storage_out,
         observer_fixture *fixture,
         uint64_t event_mask,
         event_log *log)
{
    ti_c54x_config config = {0U, {event_mask, log, record_event}};
    ti_c54x_bus bus = make_bus(fixture);
    ti_c54x_cpu *cpu = NULL;
    void *storage = malloc(ti_c54x_cpu_size());

    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    *storage_out = storage;
    return cpu;
}

static void
test_mask_zero_has_no_events(void)
{
    observer_fixture fixture = {0x1000U, 0U};
    event_log log = {0};
    void *storage = NULL;
    ti_c54x_cpu *cpu = make_cpu(&storage, &fixture, UINT64_C(0), &log);
    ti_c54x_run_result result = ti_c54x_cpu_step(cpu);

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNIMPLEMENTED_OPCODE);
    TI_C54X_TEST_CHECK(log.count == 0U);
    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_selected_events_preserve_order_and_unknown_bits(void)
{
    observer_fixture fixture = {0x1000U, 0U};
    event_log log = {0};
    void *storage = NULL;
    ti_c54x_cpu *cpu = make_cpu(&storage, &fixture, UINT64_MAX, &log);

    TI_C54X_TEST_CHECK(log.count == 1U);
    TI_C54X_TEST_CHECK(log.kinds[0] == TI_C54X_TRACE_RESET);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_step(cpu).reason ==
                       TI_C54X_STOP_UNIMPLEMENTED_OPCODE);
    TI_C54X_TEST_CHECK(log.count == 3U);
    TI_C54X_TEST_CHECK(log.kinds[1] == TI_C54X_TRACE_FETCH);
    TI_C54X_TEST_CHECK(log.kinds[2] == TI_C54X_TRACE_STOP);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(log.count == 4U);
    TI_C54X_TEST_CHECK(log.kinds[3] == TI_C54X_TRACE_RESET);
    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_instances_have_independent_masks(void)
{
    observer_fixture first_fixture = {0x1111U, 0U};
    observer_fixture second_fixture = {0x2222U, 0U};
    event_log first_log = {0};
    event_log second_log = {0};
    void *first_storage = NULL;
    void *second_storage = NULL;
    ti_c54x_cpu *first = make_cpu(&first_storage, &first_fixture,
                                  TI_C54X_TRACE_MASK_FETCH, &first_log);
    ti_c54x_cpu *second = make_cpu(&second_storage, &second_fixture,
                                   TI_C54X_TRACE_MASK_STOP, &second_log);

    TI_C54X_TEST_CHECK(first_log.count == 0U);
    TI_C54X_TEST_CHECK(second_log.count == 0U);
    (void)ti_c54x_cpu_step(first);
    (void)ti_c54x_cpu_step(second);
    TI_C54X_TEST_CHECK(first_log.count == 1U);
    TI_C54X_TEST_CHECK(first_log.kinds[0] == TI_C54X_TRACE_FETCH);
    TI_C54X_TEST_CHECK(second_log.count == 1U);
    TI_C54X_TEST_CHECK(second_log.kinds[0] == TI_C54X_TRACE_STOP);
    ti_c54x_cpu_deinit(first);
    ti_c54x_cpu_deinit(second);
    free(first_storage);
    free(second_storage);
}

static void
test_observation_does_not_change_architectural_result(void)
{
    observer_fixture first_fixture = {UINT16_C(0xEBAB), 0U};
    observer_fixture second_fixture = {UINT16_C(0xEBAB), 0U};
    event_log first_log = {0};
    event_log second_log = {0};
    ti_c54x_state first_state;
    ti_c54x_state second_state;
    void *first_storage = NULL;
    void *second_storage = NULL;
    ti_c54x_cpu *first = make_cpu(&first_storage, &first_fixture,
                                  TI_C54X_TRACE_MASK_ALL, &first_log);
    ti_c54x_cpu *second = make_cpu(&second_storage, &second_fixture,
                                   UINT64_C(0), &second_log);
    ti_c54x_run_result first_result = ti_c54x_cpu_step(first);
    ti_c54x_run_result second_result = ti_c54x_cpu_step(second);

    TI_C54X_TEST_CHECK(first_result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(first_result.reason == second_result.reason);
    TI_C54X_TEST_CHECK(first_result.bus_status == second_result.bus_status);
    TI_C54X_TEST_CHECK(first_result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(first_result.pc_word == second_result.pc_word);
    TI_C54X_TEST_CHECK(first_result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(first_result.opcode_word == second_result.opcode_word);
    TI_C54X_TEST_CHECK(first_result.opcode_word == UINT16_C(0xEBAB));
    TI_C54X_TEST_CHECK(first_result.opcode_valid == second_result.opcode_valid);
    TI_C54X_TEST_CHECK(first_result.opcode_valid);
    TI_C54X_TEST_CHECK(first_result.instructions_attempted ==
                       second_result.instructions_attempted);
    TI_C54X_TEST_CHECK(first_result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(first_result.instructions_retired ==
                       second_result.instructions_retired);
    TI_C54X_TEST_CHECK(first_result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(first_log.count == 2U);
    TI_C54X_TEST_CHECK(first_log.kinds[0] == TI_C54X_TRACE_RESET);
    TI_C54X_TEST_CHECK(first_log.kinds[1] == TI_C54X_TRACE_FETCH);
    TI_C54X_TEST_CHECK(second_log.count == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(first, &first_state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(second, &second_state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(first_state.pc_word == second_state.pc_word);
    TI_C54X_TEST_CHECK(first_state.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(first_state.last_fetched_word ==
                       second_state.last_fetched_word);
    TI_C54X_TEST_CHECK(first_state.last_fetched_word == UINT16_C(0xEBAB));
    TI_C54X_TEST_CHECK(first_state.last_fetch_valid ==
                       second_state.last_fetch_valid);
    TI_C54X_TEST_CHECK(first_state.last_fetch_valid);
    TI_C54X_TEST_CHECK(first_state.instructions_attempted_total ==
                       second_state.instructions_attempted_total);
    TI_C54X_TEST_CHECK(first_state.instructions_attempted_total ==
                       UINT64_C(1));
    TI_C54X_TEST_CHECK(first_state.instructions_retired_total ==
                       second_state.instructions_retired_total);
    TI_C54X_TEST_CHECK(first_state.instructions_retired_total == UINT64_C(1));
    TI_C54X_TEST_CHECK(first_state.dp == second_state.dp);
    TI_C54X_TEST_CHECK(first_state.dp == UINT16_C(0x01AB));
    TI_C54X_TEST_CHECK(first_state.dp_known == second_state.dp_known);
    TI_C54X_TEST_CHECK(first_state.dp_known);
    TI_C54X_TEST_CHECK(first_state.asm_raw == second_state.asm_raw);
    TI_C54X_TEST_CHECK(first_state.asm_known == second_state.asm_known);
    TI_C54X_TEST_CHECK(first_state.arp == second_state.arp);
    TI_C54X_TEST_CHECK(first_state.arp_known == second_state.arp_known);
    TI_C54X_TEST_CHECK(first_state.accumulator_a_raw40 ==
                       second_state.accumulator_a_raw40);
    TI_C54X_TEST_CHECK(first_state.accumulator_a_known ==
                       second_state.accumulator_a_known);
    TI_C54X_TEST_CHECK(first_state.accumulator_b_raw40 ==
                       second_state.accumulator_b_raw40);
    TI_C54X_TEST_CHECK(first_state.accumulator_b_known ==
                       second_state.accumulator_b_known);
    TI_C54X_TEST_CHECK(first_state.c == second_state.c);
    TI_C54X_TEST_CHECK(first_state.c_known == second_state.c_known);
    ti_c54x_cpu_deinit(first);
    ti_c54x_cpu_deinit(second);
    free(first_storage);
    free(second_storage);
}

static void
test_callback_reentry_fails_closed(void)
{
    observer_fixture fixture = {UINT16_C(0xF495), 0U};
    event_log log = {0};
    void *storage = NULL;
    ti_c54x_cpu *cpu = make_cpu(&storage, &fixture, TI_C54X_TRACE_MASK_FETCH,
                                &log);
    ti_c54x_run_result result;
    ti_c54x_state state;

    log.cpu_for_reentry = cpu;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(log.reentry_attempts == 1U);
    TI_C54X_TEST_CHECK(log.nested_result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.last_fetch_valid);
    TI_C54X_TEST_CHECK(state.last_fetched_word == UINT16_C(0xF495));
    TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

int
main(void)
{
    test_mask_zero_has_no_events();
    test_selected_events_preserve_order_and_unknown_bits();
    test_instances_have_independent_masks();
    test_observation_does_not_change_architectural_result();
    test_callback_reentry_fails_closed();
    return 0;
}
