/*
 * Module contract
 *
 * Owns: end-to-end executable proof that the CPU trusted instruction path
 * performs one fetch and one retirement per one-word packet, preserves fixed
 * status-load semantics, and retains unknown-opcode stop boundaries.
 * Does not own: packet internals, family arithmetic, memory WAIT transactions,
 * timing, diagnostics, benchmark thresholds, or public ABI design.
 */

#include <ti_c54x/cpu.h>

#include "../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct trusted_cpu_bus {
    const uint16_t *words;
    size_t word_count;
    uint64_t reads;
} trusted_cpu_bus;

static ti_c54x_bus_status read_word(
    void *context,
    ti_c54x_program_word_address address_word,
    uint16_t *value_word)
{
    trusted_cpu_bus *bus = (trusted_cpu_bus *)context;

    TI_C54X_TEST_CHECK(address_word < bus->word_count);
    ++bus->reads;
    *value_word = bus->words[address_word];
    return TI_C54X_BUS_OK;
}

static void test_fixed_mixed_stream(void)
{
    static const uint16_t words[] = {
        UINT16_C(0xF495), UINT16_C(0xEBAB),
        UINT16_C(0xED1F), UINT16_C(0xF4A7)};
    trusted_cpu_bus context = {words, 4U, UINT64_C(0)};
    const ti_c54x_bus bus = {
        .context = &context, .program_read_word = read_word};
    const ti_c54x_config config = {0};
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(),
                                        &config, &bus, &cpu) ==
                       TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(4));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(4));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(4));
    TI_C54X_TEST_CHECK(context.reads == UINT64_C(4));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(4));
    TI_C54X_TEST_CHECK(state.dp == UINT16_C(0x1AB));
    TI_C54X_TEST_CHECK(state.asm_raw == UINT16_C(0x1F));
    TI_C54X_TEST_CHECK(state.arp == UINT8_C(7));
    TI_C54X_TEST_CHECK(state.dp_known);
    TI_C54X_TEST_CHECK(state.asm_known);
    TI_C54X_TEST_CHECK(state.arp_known);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    free(storage);
}

static void test_unknown_stops_before_packet_dispatch(void)
{
    static const uint16_t words[] = {UINT16_C(0x1000)};
    trusted_cpu_bus context = {words, 1U, UINT64_C(0)};
    const ti_c54x_bus bus = {
        .context = &context, .program_read_word = read_word};
    const ti_c54x_config config = {0};
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(storage != NULL);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(
                           storage, ti_c54x_cpu_size(), &config,
                           &bus, &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_step(cpu);

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNIMPLEMENTED_OPCODE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(context.reads == UINT64_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    free(storage);
}

int main(void)
{
    test_fixed_mixed_stream();
    test_unknown_stops_before_packet_dispatch();
    puts("trusted_instruction_path: ok");
    return 0;
}
