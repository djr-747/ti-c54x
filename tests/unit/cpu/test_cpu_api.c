/* Module contract
 * Owns: executable tests for the public CPU lifecycle and bus contract.
 * Does not own: CPU implementation, observer behavior, arithmetic semantics,
 * or the shared test harness.
 */

#include <stdint.h>
#include <stdlib.h>

#include <ti_c54x/cpu.h>
#include "../../support/test_harness.h"

typedef struct bus_fixture {
    uint16_t opcode_word;
    ti_c54x_program_word_address expected_address;
    ti_c54x_bus_status read_status;
    unsigned reads;
} bus_fixture;

static ti_c54x_bus_status read_program(
    void *context,
    ti_c54x_program_word_address address_word,
    uint16_t *value_word)
{
    bus_fixture *fixture = (bus_fixture *)context;

    fixture->reads++;
    TI_C54X_TEST_CHECK(address_word == fixture->expected_address);
    if (fixture->read_status != TI_C54X_BUS_OK) {
        return fixture->read_status;
    }
    *value_word = fixture->opcode_word;
    return TI_C54X_BUS_OK;
}

static ti_c54x_bus make_program_bus(bus_fixture *fixture)
{
    ti_c54x_bus bus = {0};

    bus.context = fixture;
    bus.program_read_word = read_program;
    return bus;
}

static void test_two_instances_are_isolated_and_extended_pc_fetches(void)
{
    bus_fixture first_fixture = {0x1234U, UINT32_C(0x10042),
                                 TI_C54X_BUS_OK, 0U};
    bus_fixture second_fixture = {0xABCDU, UINT32_C(0x000A0),
                                  TI_C54X_BUS_OK, 0U};
    ti_c54x_bus first_bus = make_program_bus(&first_fixture);
    ti_c54x_bus second_bus = make_program_bus(&second_fixture);
    ti_c54x_config first_config = {UINT32_C(0x10042), {0U, NULL, NULL}};
    ti_c54x_config second_config = {UINT32_C(0x000A0), {0U, NULL, NULL}};
    ti_c54x_state first_state;
    ti_c54x_state second_state;
    void *first_storage = malloc(ti_c54x_cpu_size());
    void *second_storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *first = NULL;
    ti_c54x_cpu *second = NULL;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(first_storage != NULL);
    TI_C54X_TEST_CHECK(second_storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_alignment() <= _Alignof(max_align_t));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(first_storage, ti_c54x_cpu_size(),
                                        &first_config, &first_bus, &first) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(second_storage, ti_c54x_cpu_size(),
                                        &second_config, &second_bus, &second) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(first != NULL);
    TI_C54X_TEST_CHECK(second != NULL);

    result = ti_c54x_cpu_run(first, 0U);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 0U);
    TI_C54X_TEST_CHECK(first_fixture.reads == 0U);

    result = ti_c54x_cpu_run(first, 4U);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNIMPLEMENTED_OPCODE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == 0x1234U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(first_fixture.reads == 1U);
    TI_C54X_TEST_CHECK(second_fixture.reads == 0U);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(first, &first_state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(second, &second_state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(first_state.pc_word == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(first_state.last_fetched_word == 0x1234U);
    TI_C54X_TEST_CHECK(first_state.instructions_attempted_total == 1U);
    TI_C54X_TEST_CHECK(second_state.pc_word == UINT32_C(0x000A0));
    TI_C54X_TEST_CHECK(!second_state.last_fetch_valid);
    TI_C54X_TEST_CHECK(second_state.instructions_attempted_total == 0U);

    result = ti_c54x_cpu_step(first);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(first_fixture.reads == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(first, &first_state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(first_state.instructions_attempted_total == 2U);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(first) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(first, &first_state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(first_state.pc_word == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(!first_state.last_fetch_valid);
    TI_C54X_TEST_CHECK(first_state.instructions_attempted_total == 0U);

    ti_c54x_cpu_deinit(first);
    ti_c54x_cpu_deinit(second);
    free(first_storage);
    free(second_storage);
}

static void test_bus_fault_and_missing_callback(void)
{
    bus_fixture fixture = {0U, UINT32_C(0x23456), TI_C54X_BUS_FAULT, 0U};
    ti_c54x_bus bus = make_program_bus(&fixture);
    ti_c54x_config config = {UINT32_C(0x23456), {0U, NULL, NULL}};
    ti_c54x_state state;
    ti_c54x_run_result result;
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, 3U);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(!result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.reads == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
    ti_c54x_cpu_deinit(cpu);
    free(storage);

    bus = (ti_c54x_bus){0};
    storage = malloc(ti_c54x_cpu_size());
    cpu = NULL;
    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_NO_CALLBACK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void test_null_arguments_and_init_failure_output(void)
{
    bus_fixture fixture = {0U, 0U, TI_C54X_BUS_OK, 0U};
    ti_c54x_bus bus = make_program_bus(&fixture);
    ti_c54x_cpu *cpu = (ti_c54x_cpu *)(uintptr_t)UINTPTR_MAX;
    ti_c54x_state state;
    ti_c54x_run_result result;
    void *storage = malloc(ti_c54x_cpu_size());

    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), NULL, &bus,
                                        NULL) == TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(NULL, ti_c54x_cpu_size(), NULL, &bus,
                                        &cpu) == TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(cpu == NULL);
    cpu = (ti_c54x_cpu *)(uintptr_t)UINTPTR_MAX;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size() - 1U,
                                        NULL, &bus, &cpu) ==
                       TI_C54X_ERROR_STORAGE_TOO_SMALL);
    TI_C54X_TEST_CHECK(cpu == NULL);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(NULL) == TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(NULL, &state) ==
                       TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(NULL, NULL) ==
                       TI_C54X_ERROR_INVALID_ARGUMENT);
    result = ti_c54x_cpu_step(NULL);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    result = ti_c54x_cpu_run(NULL, 0U);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    ti_c54x_cpu_deinit(NULL);
    free(storage);
}

int main(void)
{
    test_two_instances_are_isolated_and_extended_pc_fetches();
    test_bus_fault_and_missing_callback();
    test_null_arguments_and_init_failure_output();
    return 0;
}
