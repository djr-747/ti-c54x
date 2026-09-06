/*
 * Module contract
 *
 * Owns: COMMON_TI proof that the public CPU can retire the catalog NOP and
 * commit the sequential common-PC transition for the highest resolved word.
 * Does not own: bridge internals, opcode catalog construction, bus mapping,
 * timing, observers, diagnostics, or multiword instruction behavior.
 */

#include <stdint.h>
#include <stdlib.h>

#include <ti_c54x/cpu.h>

#include "../../support/test_harness.h"

typedef struct nop_bus_fixture {
    unsigned reads;
    ti_c54x_program_word_address addresses[2];
} nop_bus_fixture;

static ti_c54x_bus_status
read_nop(void *context,
         ti_c54x_program_word_address address_word,
         uint16_t *value_word)
{
    nop_bus_fixture *fixture = (nop_bus_fixture *)context;

    TI_C54X_TEST_CHECK(fixture->reads < 2U);
    fixture->addresses[fixture->reads] = address_word;
    fixture->reads++;
    *value_word = UINT16_C(0xf495);
    return TI_C54X_BUS_OK;
}

static void
test_nop_retires_and_advances_from_zero(void)
{
    nop_bus_fixture fixture = {0U, {0U, 0U}};
    ti_c54x_bus bus = {0};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    ti_c54x_run_result result;
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;

    bus.context = &fixture;
    bus.program_read_word = read_nop;
    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);

    result = ti_c54x_cpu_run(cpu, 1U);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0xf495));
    TI_C54X_TEST_CHECK(fixture.reads == 1U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0));

    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_nop_retires_at_last_resolved_common_word(void)
{
    nop_bus_fixture fixture = {0U, {0U, 0U}};
    ti_c54x_bus bus = {0};
    ti_c54x_config config = {UINT32_C(0xfffe), {0U, NULL, NULL}};
    ti_c54x_run_result result;
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;

    bus.context = &fixture;
    bus.program_read_word = read_nop;
    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0xffff));
    TI_C54X_TEST_CHECK(fixture.reads == 1U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0xfffe));

    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

int
main(void)
{
    test_nop_retires_and_advances_from_zero();
    test_nop_retires_at_last_resolved_common_word();
    return 0;
}
