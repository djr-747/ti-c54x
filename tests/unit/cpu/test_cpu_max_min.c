/* Module contract
 * Owns: public MAX/MIN retirement, unknown-input, and rollback proof.
 * Does not own: CPU implementation, decode tables, or timing.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include <ti_c54x/cpu.h>
#include "../../../src/cpu/cpu_internal.h"
#include "../../../src/state/accumulator/accumulator.h"
#include "../../support/test_harness.h"

typedef struct fixture { uint16_t word; uint32_t address; } fixture;

static ti_c54x_bus_status read_word(void *context,
                                    ti_c54x_program_word_address address,
                                    uint16_t *value)
{
    fixture *f = (fixture *)context;
    TI_C54X_TEST_CHECK(address == f->address);
    *value = f->word;
    return TI_C54X_BUS_OK;
}

static void run_case(uint16_t word, uint64_t a, uint64_t b, uint64_t expected,
                     uint8_t expected_c, uint8_t destination)
{
    fixture f = {word, 0U};
    ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_size() <= sizeof(storage));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A, a) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B, b) ==
                       TI_C54X_ACCUMULATOR_OK);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(result.opcode_word == word);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == 1U);
    TI_C54X_TEST_CHECK(state.last_fetched_word == word);
    TI_C54X_TEST_CHECK(state.last_fetch_valid);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 1U);
    TI_C54X_TEST_CHECK(state.accumulator_a_known);
    TI_C54X_TEST_CHECK(state.accumulator_b_known);
    TI_C54X_TEST_CHECK_U64(destination == 0U ? state.accumulator_a_raw40
                                             : state.accumulator_b_raw40,
                           expected);
    TI_C54X_TEST_CHECK_U64(destination == 0U ? state.accumulator_b_raw40
                                             : state.accumulator_a_raw40,
                           destination == 0U ? b : a);
    TI_C54X_TEST_CHECK(state.c_known);
    TI_C54X_TEST_CHECK(state.c == expected_c);
    ti_c54x_cpu_deinit(cpu);
}

int main(void)
{
    run_case(UINT16_C(0xF486), 8U, 4U, 8U, 0U, 0U);
    run_case(UINT16_C(0xF586), 8U, 4U, 8U, 0U, 1U);
    run_case(UINT16_C(0xF487), 8U, 4U, 4U, 1U, 0U);
    run_case(UINT16_C(0xF587), 8U, 4U, 4U, 1U, 1U);
    {
        fixture f = {UINT16_C(0xF486), 0U};
        ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
        ti_c54x_config config = {0U, {0U, NULL, NULL}};
        _Alignas(max_align_t) unsigned char storage[1024];
        ti_c54x_cpu *cpu = NULL;
        ti_c54x_state state;
        ti_c54x_run_result result;
        TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                            &bus, &cpu) == TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_step(cpu);
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
        TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == 0U);
        TI_C54X_TEST_CHECK_U64(state.accumulator_a_raw40, 0U);
        TI_C54X_TEST_CHECK_U64(state.accumulator_b_raw40, 0U);
        TI_C54X_TEST_CHECK(!state.accumulator_a_known);
        TI_C54X_TEST_CHECK(!state.accumulator_b_known);
        TI_C54X_TEST_CHECK(state.c == 1U);
        TI_C54X_TEST_CHECK(state.c_known);
        TI_C54X_TEST_CHECK(state.last_fetched_word == UINT16_C(0xF486));
        TI_C54X_TEST_CHECK(state.last_fetch_valid);
        TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
        TI_C54X_TEST_CHECK(state.instructions_retired_total == 0U);
        ti_c54x_cpu_deinit(cpu);
    }
    {
        fixture f = {UINT16_C(0x0000), 0U};
        ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
        ti_c54x_config config = {0U, {0U, NULL, NULL}};
        _Alignas(max_align_t) unsigned char storage[1024];
        ti_c54x_cpu *cpu = NULL;
        ti_c54x_state state;
        ti_c54x_run_result result;
        TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                            &bus, &cpu) == TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_step(cpu);
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNIMPLEMENTED_OPCODE);
        TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
        TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
        TI_C54X_TEST_CHECK(result.opcode_valid);
        TI_C54X_TEST_CHECK(result.opcode_word == UINT16_C(0x0000));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == 0U);
        TI_C54X_TEST_CHECK(!state.accumulator_a_known);
        TI_C54X_TEST_CHECK(!state.accumulator_b_known);
        TI_C54X_TEST_CHECK(state.c == 1U);
        TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
        TI_C54X_TEST_CHECK(state.instructions_retired_total == 0U);
        ti_c54x_cpu_deinit(cpu);
    }
    {
        fixture f = {UINT16_C(0xF486), UINT32_C(0xFFFF)};
        ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
        ti_c54x_config config = {UINT32_C(0xFFFF), {0U, NULL, NULL}};
        _Alignas(max_align_t) unsigned char storage[1024];
        ti_c54x_cpu *cpu = NULL;
        ti_c54x_state state;
        ti_c54x_run_result result;
        TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                            &bus, &cpu) == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                               &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                               7U) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                               &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                               3U) == TI_C54X_ACCUMULATOR_OK);
        result = ti_c54x_cpu_step(cpu);
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
        TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
        TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK_U64(state.accumulator_a_raw40, 7U);
        TI_C54X_TEST_CHECK_U64(state.accumulator_b_raw40, 3U);
        TI_C54X_TEST_CHECK(state.accumulator_a_known);
        TI_C54X_TEST_CHECK(state.accumulator_b_known);
        TI_C54X_TEST_CHECK(state.c == 1U);
        TI_C54X_TEST_CHECK(state.c_known);
        TI_C54X_TEST_CHECK(state.last_fetched_word == UINT16_C(0xF486));
        TI_C54X_TEST_CHECK(state.last_fetch_valid);
        TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
        TI_C54X_TEST_CHECK(state.instructions_retired_total == 0U);
        TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0xFFFF));
        ti_c54x_cpu_deinit(cpu);
    }
    return 0;
}
