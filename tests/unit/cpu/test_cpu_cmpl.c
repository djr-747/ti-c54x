/* Module contract
 * Owns: public CMPL retirement, C-preservation, unknown-input, and PC
 * boundary rollback proof.
 * Does not own: CPU implementation, decode tables, or timing.
 */

#include <ti_c54x/cpu.h>

#include "../../../src/cpu/cpu_internal.h"
#include "../../../src/state/accumulator/accumulator.h"
#include "../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct fixture {
    uint16_t word;
    uint32_t address;
} fixture;

static ti_c54x_bus_status read_word(void *context,
                                    ti_c54x_program_word_address address,
                                    uint16_t *value)
{
    fixture *f = (fixture *)context;
    TI_C54X_TEST_CHECK(address == f->address);
    *value = f->word;
    return TI_C54X_BUS_OK;
}

static void run_case(uint16_t word, uint64_t a, uint64_t b,
                     uint8_t initial_c, uint64_t expected,
                     uint8_t destination)
{
    fixture f = {word, 0U};
    ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[512];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    ti_c54x_status_register_state before_status;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A, a) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B, b) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&cpu->arch_state.status,
                                                       initial_c) ==
                       TI_C54X_STATUS_REGISTER_OK);
    before_status = cpu->arch_state.status;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == 1U);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 1U);
    TI_C54X_TEST_CHECK_U64(destination == 0U ? state.accumulator_a_raw40
                                             : state.accumulator_b_raw40,
                           expected);
    TI_C54X_TEST_CHECK(state.c_known);
    TI_C54X_TEST_CHECK(state.c == initial_c);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_unknown_unselected(uint16_t word, bool known_a,
                                   uint64_t known_value, uint64_t expected)
{
    fixture f = {word, 0U};
    ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[512];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           known_a ? TI_C54X_ACCUMULATOR_A
                                   : TI_C54X_ACCUMULATOR_B,
                           known_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&cpu->arch_state.status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == 1U);
    TI_C54X_TEST_CHECK_U64(state.accumulator_a_raw40, expected);
    TI_C54X_TEST_CHECK(state.c == 1U);
    TI_C54X_TEST_CHECK(state.c_known);
    ti_c54x_cpu_deinit(cpu);
}

int main(void)
{
    run_case(UINT16_C(0xF493), UINT64_C(0x12345), UINT64_C(0xABCDE), 0U,
             UINT64_C(0xFFFFFEDCBA), 0U);
    run_case(UINT16_C(0xF693), UINT64_C(0x12345), UINT64_C(0xABCDE), 1U,
             UINT64_C(0xFFFFF54321), 0U);
    run_case(UINT16_C(0xF593), UINT64_C(0x12345), UINT64_C(0xABCDE), 0U,
             UINT64_C(0xFFFFFEDCBA), 1U);
    run_case(UINT16_C(0xF793), UINT64_C(0x12345), UINT64_C(0xABCDE), 1U,
             UINT64_C(0xFFFFF54321), 1U);
    run_unknown_unselected(UINT16_C(0xF493), true, 1U,
                           UINT64_C(0xFFFFFFFFFE));
    run_unknown_unselected(UINT16_C(0xF693), false, 2U,
                           UINT64_C(0xFFFFFFFFFD));

    {
        fixture f = {UINT16_C(0xF493), 0U};
        ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
        ti_c54x_config config = {0U, {0U, NULL, NULL}};
        _Alignas(max_align_t) unsigned char storage[512];
        ti_c54x_cpu *cpu = NULL;
        ti_c54x_state before;
        ti_c54x_state after;
        ti_c54x_run_result result;
        ti_c54x_status_register_state before_status;
        ti_c54x_accumulator_state before_accumulators;

        TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                            &bus, &cpu) == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&cpu->arch_state.status, 1U) ==
                           TI_C54X_STATUS_REGISTER_OK);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &before) ==
                           TI_C54X_ERROR_OK);
        before_status = cpu->arch_state.status;
        before_accumulators = cpu->accumulators;
        result = ti_c54x_cpu_step(cpu);
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
        TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &after) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(after.pc_word == before.pc_word);
        TI_C54X_TEST_CHECK(after.accumulator_a_known == before.accumulator_a_known);
        TI_C54X_TEST_CHECK(after.accumulator_b_known == before.accumulator_b_known);
        TI_C54X_TEST_CHECK(after.c == before.c);
        TI_C54X_TEST_CHECK(after.c_known == before.c_known);
        TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                                  sizeof(before_status)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                                  sizeof(before_accumulators)) == 0);
        ti_c54x_cpu_deinit(cpu);
    }

    {
        fixture f = {UINT16_C(0xF493), UINT32_C(0xFFFF)};
        ti_c54x_bus bus = {.context = &f, .program_read_word = read_word};
        ti_c54x_config config = {UINT32_C(0xFFFF), {0U, NULL, NULL}};
        _Alignas(max_align_t) unsigned char storage[512];
        ti_c54x_cpu *cpu = NULL;
        ti_c54x_state state;
        ti_c54x_run_result result;
        ti_c54x_status_register_state before_status;
        ti_c54x_accumulator_state before_accumulators;

        TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                            &bus, &cpu) == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                               &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                               7U) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                               &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                               3U) == TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&cpu->arch_state.status, 1U) ==
                           TI_C54X_STATUS_REGISTER_OK);
        before_status = cpu->arch_state.status;
        before_accumulators = cpu->accumulators;
        result = ti_c54x_cpu_step(cpu);
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
        TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
        TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0xFFFF));
        TI_C54X_TEST_CHECK_U64(state.accumulator_a_raw40, 7U);
        TI_C54X_TEST_CHECK_U64(state.accumulator_b_raw40, 3U);
        TI_C54X_TEST_CHECK(state.c == 1U);
        TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                                  sizeof(before_status)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                                  sizeof(before_accumulators)) == 0);
        ti_c54x_cpu_deinit(cpu);
    }
    return 0;
}
