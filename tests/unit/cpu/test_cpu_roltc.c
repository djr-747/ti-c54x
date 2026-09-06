/* Module contract
 * Owns: public ROLTC retirement, rotate-through-TC semantics, C output,
 * unknown-input rollback, status preservation, and program-boundary proof.
 * Does not own: CPU implementation, decode tables, opcode handlers, timing,
 * or input collection implementation.
 */

#include <ti_c54x/cpu.h>

#include "../../../src/cpu/cpu_internal.h"
#include "../../../src/state/accumulator/accumulator.h"
#include "../../../src/state/register_layout.h"
#include "../../../src/state/status/registers.h"
#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct cpu_fixture {
    uint16_t word;
    ti_c54x_program_word_address expected_address;
} cpu_fixture;

static ti_c54x_bus_status read_word(void *context,
                                    ti_c54x_program_word_address address,
                                    uint16_t *value)
{
    cpu_fixture *fixture = (cpu_fixture *)context;

    TI_C54X_TEST_CHECK(address == fixture->expected_address);
    *value = fixture->word;
    return TI_C54X_BUS_OK;
}

static uint64_t expected_value(uint64_t source, uint8_t tc, uint8_t *out_c)
{
    const uint64_t low32 = source & UINT64_C(0xFFFFFFFF);

    *out_c = (uint8_t)((low32 >> 31U) & UINT64_C(1));
    return ((low32 << 1U) & UINT64_C(0xFFFFFFFF)) | (uint64_t)tc;
}

static void seed_non_c_status(ti_c54x_status_register_state *status)
{
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_dp(status, 0x155U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_asm(status, 0x12U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_arp(status, 5U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_cpl(status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_cmpt(status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
}

static void clear_c_fields(ti_c54x_status_register_state *status)
{
    status->st0 = (uint16_t)(status->st0 & (uint16_t)~TI_C54X_ST0_C_MASK);
    status->st0_known_mask = (uint16_t)(
        status->st0_known_mask & (uint16_t)~TI_C54X_ST0_C_MASK);
}

static void run_case(uint16_t word, uint64_t a, uint64_t b, uint8_t initial_tc,
                     uint8_t initial_c, bool initial_c_known)
{
    cpu_fixture fixture = {word, 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[256];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    ti_c54x_status_register_state before_status;
    const uint8_t selected_id = (uint8_t)((word >> 8U) & UINT16_C(1));
    const uint64_t selected = selected_id == 0U ? a : b;
    const uint64_t expected_other = selected_id == 0U ? b : a;
    uint8_t expected_c = 0U;
    const uint64_t expected = expected_value(selected, initial_tc, &expected_c);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A, a) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B, b) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(&cpu->status,
                                                       initial_tc) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&cpu->status,
                                                       initial_c) ==
                       TI_C54X_STATUS_REGISTER_OK);
    seed_non_c_status(&cpu->status);
    if (!initial_c_known) {
        cpu->status.st0_known_mask = (uint16_t)(
            cpu->status.st0_known_mask & (uint16_t)~TI_C54X_ST0_C_MASK);
    }
    before_status = cpu->status;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == 1U);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 1U);
    TI_C54X_TEST_CHECK_U64(selected_id == 0U ? state.accumulator_a_raw40
                                             : state.accumulator_b_raw40,
                           expected);
    TI_C54X_TEST_CHECK_U64(selected_id == 0U ? state.accumulator_b_raw40
                                             : state.accumulator_a_raw40,
                           expected_other);
    TI_C54X_TEST_CHECK(state.c_known);
    TI_C54X_TEST_CHECK(state.c == expected_c);
    {
        const ti_c54x_status_register_field_result tc =
            ti_c54x_status_register_read_tc(&cpu->status);
        TI_C54X_TEST_CHECK(tc.status == TI_C54X_STATUS_REGISTER_OK);
        TI_C54X_TEST_CHECK(tc.known);
        TI_C54X_TEST_CHECK(tc.raw_value == initial_tc);
    }
    clear_c_fields(&before_status);
    clear_c_fields(&cpu->status);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->status,
                              sizeof(before_status)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_unknown_selected(uint8_t source)
{
    const uint16_t word = source == 0U ? UINT16_C(0xF492) : UINT16_C(0xF592);
    cpu_fixture fixture = {word, 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[256];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           source == 0U ? TI_C54X_ACCUMULATOR_B
                                        : TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x123456789A)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(&cpu->status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    before_status = cpu->status;
    before_accumulators = cpu->accumulators;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(cpu->pc_word == 0U);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_unknown_tc(void)
{
    cpu_fixture fixture = {UINT16_C(0xF492), 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[256];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x123456789A)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0ABCDEFFF0)) ==
                       TI_C54X_ACCUMULATOR_OK);
    cpu->status.st0_known_mask = (uint16_t)(
        cpu->status.st0_known_mask & (uint16_t)~TI_C54X_ST0_TC_MASK);
    before_status = cpu->status;
    before_accumulators = cpu->accumulators;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(cpu->pc_word == 0U);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_unselected_unknown(uint8_t source)
{
    const uint16_t word = source == 0U ? UINT16_C(0xF492) : UINT16_C(0xF592);
    cpu_fixture fixture = {word, 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[256];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    uint8_t expected_c = 0U;
    const uint64_t expected = expected_value(
        UINT64_C(0x123456789A), UINT8_C(1), &expected_c);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           source == 0U ? TI_C54X_ACCUMULATOR_A
                                        : TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x123456789A)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(&cpu->status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK_U64(source == 0U ? state.accumulator_a_raw40
                                        : state.accumulator_b_raw40,
                           expected);
    TI_C54X_TEST_CHECK(source == 0U ? !state.accumulator_b_known
                                    : !state.accumulator_a_known);
    TI_C54X_TEST_CHECK(state.c_known);
    TI_C54X_TEST_CHECK(state.c == expected_c);
    TI_C54X_TEST_CHECK(state.pc_word == 1U);
    ti_c54x_cpu_deinit(cpu);
}

static void run_program_boundary(void)
{
    cpu_fixture fixture = {UINT16_C(0xF492), UINT32_C(0xFFFF)};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {UINT32_C(0xFFFF), {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[256];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0xABCDE12345)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(&cpu->status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    before_status = cpu->status;
    before_accumulators = cpu->accumulators;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason ==
                       TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(cpu->pc_word == UINT32_C(0xFFFF));
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

int main(void)
{
    static const uint64_t vectors[] = {
        UINT64_C(0x0000000000), UINT64_C(0x0000000001),
        UINT64_C(0x0000000007), UINT64_C(0x000000007FFFFFFF),
        UINT64_C(0x0000000080000000), UINT64_C(0x00000000FFFFFFFF),
        UINT64_C(0xFF00000000), UINT64_C(0xFF80000000),
        UINT64_C(0x123456789A), UINT64_C(0x00ABCDEF01)};
    size_t index;
    uint8_t tc;

    for (tc = UINT8_C(0); tc <= UINT8_C(1); ++tc) {
        for (index = 0U; index < sizeof(vectors) / sizeof(vectors[0]);
             ++index) {
            run_case(UINT16_C(0xF492), vectors[index],
                     UINT64_C(0x13579BDF02), tc, UINT8_C(0), false);
            run_case(UINT16_C(0xF592), UINT64_C(0x02468ACE10), vectors[index],
                     tc, UINT8_C(1), true);
        }
    }

    run_unknown_selected(0U);
    run_unknown_selected(1U);
    run_unknown_tc();
    run_unselected_unknown(0U);
    run_unselected_unknown(1U);
    run_program_boundary();
    return 0;
}
