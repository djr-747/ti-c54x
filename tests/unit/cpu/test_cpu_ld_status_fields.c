/*
 * Module contract
 *
 * Owns: public CPU end-to-end proof for the three immediate status-field
 * loads. It verifies one fetch/dispatch/retire, sequential PC movement,
 * public field observations, knownness, reset, and unknown-word stopping.
 * Does not own: CPU internals, opcode tables, state storage, timing,
 * memory/data/I/O callbacks, repeat/interrupt policy, or diagnostics.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti_c54x/cpu.h>

#include "../../support/test_harness.h"

typedef struct ld_cpu_fixture {
    uint16_t word;
    ti_c54x_program_word_address expected_address;
    unsigned reads;
} ld_cpu_fixture;

static ti_c54x_bus_status
read_word(void *context,
          ti_c54x_program_word_address address_word,
          uint16_t *value_word)
{
    ld_cpu_fixture *fixture = (ld_cpu_fixture *)context;

    TI_C54X_TEST_CHECK(address_word == fixture->expected_address);
    TI_C54X_TEST_CHECK(fixture->reads == 0U);
    fixture->reads++;
    *value_word = fixture->word;
    return TI_C54X_BUS_OK;
}

static void
run_and_check(uint16_t word,
              uint16_t expected_dp,
              uint16_t expected_asm,
              uint16_t expected_arp,
              bool expect_dp,
              bool expect_asm,
              bool expect_arp)
{
    ld_cpu_fixture fixture = {word, UINT32_C(0), 0U};
    ti_c54x_bus bus = {0};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    ti_c54x_state state;
    ti_c54x_run_result result;
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;

    bus.context = &fixture;
    bus.program_read_word = read_word;
    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.reads == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(1));
    TI_C54X_TEST_CHECK(state.dp == expected_dp);
    TI_C54X_TEST_CHECK(state.asm_raw == expected_asm);
    TI_C54X_TEST_CHECK(state.arp == expected_arp);
    TI_C54X_TEST_CHECK(state.dp_known == expect_dp);
    TI_C54X_TEST_CHECK(state.asm_known == expect_asm);
    TI_C54X_TEST_CHECK(state.arp_known == expect_arp);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(state.dp == UINT16_C(0));
    TI_C54X_TEST_CHECK(state.asm_raw == UINT16_C(0));
    TI_C54X_TEST_CHECK(state.arp == UINT16_C(0));
    TI_C54X_TEST_CHECK(state.dp_known);
    TI_C54X_TEST_CHECK(state.asm_known);
    TI_C54X_TEST_CHECK(state.arp_known);

    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_unknown_word_is_not_retired(void)
{
    ld_cpu_fixture fixture = {UINT16_C(0x0000), UINT32_C(0), 0U};
    ti_c54x_bus bus = {0};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    ti_c54x_run_result result;
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;

    bus.context = &fixture;
    bus.program_read_word = read_word;
    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNIMPLEMENTED_OPCODE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0));
    ti_c54x_cpu_deinit(cpu);
    free(storage);
}

static void
test_status_effect_rolls_back_when_pc_boundary_cannot_retire(void)
{
    const uint16_t words[] = {UINT16_C(0xEA00) | UINT16_C(0x01FF),
                              UINT16_C(0xED00) | UINT16_C(0x001F),
                              UINT16_C(0xF4A0) | UINT16_C(0x0007)};

    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); ++i) {
        ld_cpu_fixture fixture = {words[i], UINT32_C(0xFFFF), 0U};
        ti_c54x_bus bus = {0};
        ti_c54x_config config = {UINT32_C(0xFFFF), {0U, NULL, NULL}};
        ti_c54x_state state;
        ti_c54x_run_result result;
        void *storage = malloc(ti_c54x_cpu_size());
        ti_c54x_cpu *cpu = NULL;

        bus.context = &fixture;
        bus.program_read_word = read_word;
        TI_C54X_TEST_CHECK(storage != NULL);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(),
                                            &config, &bus, &cpu) ==
                           TI_C54X_ERROR_OK);

        result = ti_c54x_cpu_step(cpu);
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0xFFFF));
        TI_C54X_TEST_CHECK(fixture.reads == 1U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0xFFFF));
        TI_C54X_TEST_CHECK(state.instructions_attempted_total == UINT64_C(1));
        TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(0));
        TI_C54X_TEST_CHECK(state.dp == UINT16_C(0));
        TI_C54X_TEST_CHECK(state.asm_raw == UINT16_C(0));
        TI_C54X_TEST_CHECK(state.arp == UINT8_C(0));
        TI_C54X_TEST_CHECK(state.dp_known);
        TI_C54X_TEST_CHECK(state.asm_known);
        TI_C54X_TEST_CHECK(state.arp_known);

        ti_c54x_cpu_deinit(cpu);
        free(storage);
    }
}

int
main(void)
{
    run_and_check(UINT16_C(0xEA00) | UINT16_C(0x01FF), UINT16_C(0x01FF),
                  UINT16_C(0), UINT16_C(0), true, true, true);
    run_and_check(UINT16_C(0xED00) | UINT16_C(0x001F), UINT16_C(0),
                  UINT16_C(0x001F), UINT16_C(0), true, true, true);
    run_and_check(UINT16_C(0xF4A0) | UINT16_C(0x0007), UINT16_C(0),
                  UINT16_C(0), UINT16_C(0x0007), true, true, true);
    test_unknown_word_is_not_retired();
    test_status_effect_rolls_back_when_pc_boundary_cannot_retire();
    return 0;
}
