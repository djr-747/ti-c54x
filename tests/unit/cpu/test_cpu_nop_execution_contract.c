/*
 * Module contract
 *
 * Owns: project-contract proof for exact CPU fetch/decode/dispatch/retire
 * behavior, known-instruction high-carrier rejection, unresolved boundaries,
 * bus failures, and transactional stop reporting.
 * Does not own: bridge implementation, opcode catalog generation, timing,
 * observer policy, diagnostics, or emulator integration.
 */

#include <stdint.h>
#include <stdlib.h>

#include <ti_c54x/cpu.h>

#include "../../support/test_harness.h"

_Static_assert(TI_C54X_STOP_BUDGET_EXHAUSTED == 0,
               "existing budget stop value changed");
_Static_assert(TI_C54X_STOP_UNIMPLEMENTED_OPCODE == 1,
               "existing unimplemented stop value changed");
_Static_assert(TI_C54X_STOP_BUS_FAULT == 2,
               "existing bus stop value changed");
_Static_assert(TI_C54X_STOP_IDLE == 3,
               "existing idle stop value changed");
_Static_assert(TI_C54X_STOP_BREAKPOINT == 4,
               "existing breakpoint stop value changed");
_Static_assert(TI_C54X_STOP_INVALID_STATE == 5,
               "existing invalid-state stop value changed");
_Static_assert(TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS == 6,
               "unsupported-address stop value is appended");
_Static_assert(TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY == 7,
               "unresolved-boundary stop value is appended");
_Static_assert(TI_C54X_STOP_COUNTER_OVERFLOW == 8,
               "counter-overflow stop value is appended");

typedef struct contract_bus_fixture {
    uint16_t words[2];
    ti_c54x_bus_status statuses[2];
    ti_c54x_program_word_address addresses[2];
    unsigned reads;
} contract_bus_fixture;

static void
test_stop_reason_numeric_values(void)
{
    TI_C54X_TEST_CHECK((int)TI_C54X_STOP_BUDGET_EXHAUSTED == 0);
    TI_C54X_TEST_CHECK((int)TI_C54X_STOP_UNIMPLEMENTED_OPCODE == 1);
    TI_C54X_TEST_CHECK((int)TI_C54X_STOP_BUS_FAULT == 2);
    TI_C54X_TEST_CHECK((int)TI_C54X_STOP_IDLE == 3);
    TI_C54X_TEST_CHECK((int)TI_C54X_STOP_BREAKPOINT == 4);
    TI_C54X_TEST_CHECK((int)TI_C54X_STOP_INVALID_STATE == 5);
    TI_C54X_TEST_CHECK((int)TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS == 6);
    TI_C54X_TEST_CHECK((int)TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY == 7);
    TI_C54X_TEST_CHECK((int)TI_C54X_STOP_COUNTER_OVERFLOW == 8);
}

static ti_c54x_bus_status
read_contract(void *context,
              ti_c54x_program_word_address address_word,
              uint16_t *value_word)
{
    contract_bus_fixture *fixture = (contract_bus_fixture *)context;
    const unsigned index = fixture->reads;

    TI_C54X_TEST_CHECK(index < 2U);
    fixture->addresses[index] = address_word;
    fixture->reads++;
    if (fixture->statuses[index] != TI_C54X_BUS_OK) {
        return fixture->statuses[index];
    }
    *value_word = fixture->words[index];
    return TI_C54X_BUS_OK;
}

static ti_c54x_run_result
run_fixture(ti_c54x_program_word_address reset_pc,
            contract_bus_fixture *fixture,
            ti_c54x_bus *bus,
            void **storage_out,
            ti_c54x_cpu **cpu_out)
{
    ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_run_result result;

    *storage_out = malloc(ti_c54x_cpu_size());
    TI_C54X_TEST_CHECK(*storage_out != NULL);
    bus->context = fixture;
    bus->program_read_word = read_contract;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(*storage_out, ti_c54x_cpu_size(),
                                        &config, bus, cpu_out) ==
                       TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_step(*cpu_out);
    return result;
}

static void
test_exact_fetch_and_unknown_low_word(void)
{
    contract_bus_fixture fixture = {{UINT16_C(0x1234), 0U},
                                    {TI_C54X_BUS_OK, TI_C54X_BUS_OK},
                                    {0U, 0U},
                                    0U};
    ti_c54x_bus bus = {0};
    void *storage = NULL;
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result = run_fixture(0U, &fixture, &bus, &storage, &cpu);

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNIMPLEMENTED_OPCODE);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(fixture.reads == 1U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0));
    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_unknown_high_word_is_fetched_exactly(void)
{
    contract_bus_fixture fixture = {{UINT16_C(0x1234), 0U},
                                    {TI_C54X_BUS_OK, TI_C54X_BUS_OK},
                                    {0U, 0U},
                                    0U};
    ti_c54x_bus bus = {0};
    void *storage = NULL;
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result = run_fixture(UINT32_C(0x10042), &fixture, &bus,
                                            &storage, &cpu);

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNIMPLEMENTED_OPCODE);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(fixture.reads == 1U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0x10042));
    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_high_nop_stops_after_exact_carrier_fetch(void)
{
    contract_bus_fixture fixture = {{UINT16_C(0xf495), 0U},
                                    {TI_C54X_BUS_OK, TI_C54X_BUS_OK},
                                    {0U, 0U},
                                    0U};
    ti_c54x_bus bus = {0};
    void *storage = NULL;
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result = run_fixture(UINT32_C(0x10042), &fixture, &bus,
                                            &storage, &cpu);

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNSUPPORTED_PROGRAM_ADDRESS);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x10042));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0xf495));
    TI_C54X_TEST_CHECK(fixture.reads == 1U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0x10042));
    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_bus_statuses_and_missing_callback_do_not_retry(void)
{
    const ti_c54x_bus_status statuses[] = {TI_C54X_BUS_FAULT,
                                            TI_C54X_BUS_UNMAPPED};
    size_t index;

    for (index = 0U; index < sizeof(statuses) / sizeof(statuses[0]); ++index) {
        contract_bus_fixture fixture = {{UINT16_C(0xf495), 0U},
                                        {statuses[index], TI_C54X_BUS_OK},
                                        {0U, 0U},
                                        0U};
        ti_c54x_bus bus = {0};
        void *storage = NULL;
        ti_c54x_cpu *cpu = NULL;
        ti_c54x_run_result result = run_fixture(0U, &fixture, &bus, &storage,
                                                &cpu);

        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.bus_status == statuses[index]);
        TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
        TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
        TI_C54X_TEST_CHECK(!result.opcode_valid);
        TI_C54X_TEST_CHECK(fixture.reads == 1U);
        ti_c54x_cpu_deinit(cpu);
        free(storage);
    }

    {
        ti_c54x_bus bus = {0};
        ti_c54x_config config = {0U, {0U, NULL, NULL}};
        void *storage = malloc(ti_c54x_cpu_size());
        ti_c54x_cpu *cpu = NULL;
        ti_c54x_run_result result;

        TI_C54X_TEST_CHECK(storage != NULL);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(),
                                            &config, &bus, &cpu) ==
                           TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_step(cpu);
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_NO_CALLBACK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
        TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
        TI_C54X_TEST_CHECK(!result.opcode_valid);
        ti_c54x_cpu_deinit(cpu);
        free(storage);
    }
}

static void
test_boundary_stop_and_last_fetch_mapping(void)
{
    contract_bus_fixture fixture = {{UINT16_C(0xf495), 0U},
                                    {TI_C54X_BUS_OK, TI_C54X_BUS_OK},
                                    {0U, 0U},
                                    0U};
    ti_c54x_bus bus = {0};
    ti_c54x_config config = {UINT32_C(0xffff), {0U, NULL, NULL}};
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;
    ti_c54x_state state;

    TI_C54X_TEST_CHECK(storage != NULL);
    bus.context = &fixture;
    bus.program_read_word = read_contract;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0xffff));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0xf495));
    TI_C54X_TEST_CHECK(fixture.reads == 1U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0xffff));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.last_fetch_valid);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 0U);
    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_zero_budget_is_side_effect_free(void)
{
    contract_bus_fixture fixture = {{UINT16_C(0xf495), 0U},
                                    {TI_C54X_BUS_OK, TI_C54X_BUS_OK},
                                    {0U, 0U},
                                    0U};
    ti_c54x_bus bus = {0};
    ti_c54x_config config = {UINT32_C(0x3456), {0U, NULL, NULL}};
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(storage != NULL);
    bus.context = &fixture;
    bus.program_read_word = read_contract;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, 0U);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 0U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x3456));
    TI_C54X_TEST_CHECK(!result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.reads == 0U);
    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

int
main(void)
{
    test_stop_reason_numeric_values();
    test_exact_fetch_and_unknown_low_word();
    test_unknown_high_word_is_fetched_exactly();
    test_high_nop_stops_after_exact_carrier_fetch();
    test_bus_statuses_and_missing_callback_do_not_retry();
    test_boundary_stop_and_last_fetch_mapping();
    test_zero_budget_is_side_effect_free();
    return 0;
}
