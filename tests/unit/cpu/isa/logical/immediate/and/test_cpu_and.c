/* Module contract
 * Owns: public-CPU proofs for two-word immediate AND fetch, extension
 * transactionality, retirement/PC boundaries, selected-source semantics, and
 * byte-for-byte status preservation.
 * Does not own: CPU implementation, catalog/decode tables, pure AND effects,
 * operand-layout extraction, parent/shared manifests, timing, repeat/IRQ
 * policy, diagnostics, or emulator integration.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../../../src/cpu/cpu_internal.h"
#include "../../../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../../../src/state/status/registers.h"
#include "../../../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct and_cpu_bus_fixture {
    uint16_t words[2];
    ti_c54x_program_word_address first_address;
    ti_c54x_program_word_address addresses[2];
    unsigned calls;
    bool fail_extension;
    ti_c54x_bus_status extension_status;
} and_cpu_bus_fixture;

static ti_c54x_bus_status read_word(void *context,
                                    ti_c54x_program_word_address address,
                                    uint16_t *value)
{
    and_cpu_bus_fixture *fixture = (and_cpu_bus_fixture *)context;

    TI_C54X_TEST_CHECK(fixture->calls < 2U);
    TI_C54X_TEST_CHECK(address == fixture->first_address + fixture->calls);
    fixture->addresses[fixture->calls] = address;
    if (fixture->calls == 1U && fixture->fail_extension) {
        fixture->calls++;
        *value = UINT16_C(0xDEAD);
        return fixture->extension_status;
    }
    *value = fixture->words[fixture->calls];
    fixture->calls++;
    return TI_C54X_BUS_OK;
}

static uint16_t shifted_word(uint8_t source, uint8_t destination,
                             uint8_t shift)
{
    return (uint16_t)(UINT16_C(0xF030) | ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U) | shift);
}

static uint16_t fixed_word(uint8_t source, uint8_t destination)
{
    return (uint16_t)(UINT16_C(0xF063) | ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U));
}

static uint64_t expected_value(uint64_t source, uint16_t lk, uint8_t shift)
{
    return source & (((uint64_t)lk << shift) & TI_C54X_U40_MASK);
}

static void seed_status(ti_c54x_status_register_state *status)
{
    status->st0 = UINT16_C(0xA5C3);
    status->st1 = UINT16_C(0x5A96);
    status->pmst = UINT16_C(0xC35A);
    status->st0_known_mask = UINT16_C(0xFFFF);
    status->st1_known_mask = UINT16_C(0xFFFF);
    status->pmst_known_mask = UINT16_C(0xFFFF);
}

static ti_c54x_cpu *init_cpu(and_cpu_bus_fixture *fixture,
                             ti_c54x_program_word_address start,
                             unsigned char *storage, size_t storage_size)
{
    const ti_c54x_bus bus = {
        .context = fixture, .program_read_word = read_word};
    const ti_c54x_config config = {start, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_size() <= storage_size);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, storage_size, &config, &bus,
                                        &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

static void run_success(uint16_t first_word, uint16_t lk, uint8_t source,
                        uint8_t destination, uint8_t shift,
                        ti_c54x_program_word_address start)
{
    const uint64_t a = UINT64_C(0xF123456789);
    const uint64_t b = UINT64_C(0x00ABCDEF01);
    and_cpu_bus_fixture fixture = {{first_word, lk}, start, {0U, 0U}, 0U,
                                   false, TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu;
    ti_c54x_status_register_state before_status;
    ti_c54x_state state;
    ti_c54x_run_result result;
    const uint64_t selected = source == 0U ? a : b;

    cpu = init_cpu(&fixture, start, storage, sizeof(storage));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A, a) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B, b) ==
                       TI_C54X_ACCUMULATOR_OK);
    seed_status(&cpu->arch_state.status);
    before_status = cpu->arch_state.status;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == first_word);
    TI_C54X_TEST_CHECK(fixture.calls == 2U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == start);
    TI_C54X_TEST_CHECK(fixture.addresses[1] == start + UINT32_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == start + UINT32_C(2));
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 1U);
    TI_C54X_TEST_CHECK_U64(destination == 0U ? state.accumulator_a_raw40
                                             : state.accumulator_b_raw40,
                           expected_value(selected, lk, shift));
    TI_C54X_TEST_CHECK_U64(destination == 0U ? state.accumulator_b_raw40
                                             : state.accumulator_a_raw40,
                           destination == 0U ? b : a);
    TI_C54X_TEST_CHECK(destination == 0U ? state.accumulator_a_known
                                         : state.accumulator_b_known);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_distinct_unknown_destination(void)
{
    const uint8_t source = 0U;
    const uint8_t destination = 1U;
    const uint8_t shift = UINT8_C(15);
    const uint16_t lk = UINT16_C(0xFFFF);
    const uint64_t source_value = UINT64_C(0x123456789A);
    and_cpu_bus_fixture fixture = {
        {shifted_word(source, destination, shift), lk}, 0U, {0U, 0U}, 0U,
        false, TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu;
    ti_c54x_state state;
    ti_c54x_run_result result;

    cpu = init_cpu(&fixture, 0U, storage, sizeof(storage));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           source_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0xDEADBEEF01)) == TI_C54X_ACCUMULATOR_OK);
    cpu->accumulators.known_mask = UINT8_C(1);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK_U64(state.accumulator_b_raw40,
                           expected_value(source_value, lk, shift));
    TI_C54X_TEST_CHECK(state.accumulator_b_known);
    ti_c54x_cpu_deinit(cpu);
}

static void run_extension_failure(void)
{
    const uint16_t first_word = shifted_word(1U, 0U, UINT8_C(1));
    and_cpu_bus_fixture fixture = {{first_word, UINT16_C(0x7FFF)}, 0U,
                                   {0U, 0U}, 0U, true, TI_C54X_BUS_FAULT};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_state state;
    ti_c54x_run_result result;

    cpu = init_cpu(&fixture, 0U, storage, sizeof(storage));
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x123456789A)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x00ABCDEF01)) == TI_C54X_ACCUMULATOR_OK);
    seed_status(&cpu->arch_state.status);
    before_status = cpu->arch_state.status;
    before_accumulators = cpu->accumulators;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(fixture.calls == 2U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == 0U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 0U);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_boundary_fffd_and_fffe(void)
{
    {
        and_cpu_bus_fixture fixture = {
            {shifted_word(0U, 0U, 0U), UINT16_C(1)}, UINT32_C(0xFFFD),
            {0U, 0U}, 0U, false, TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024];
        ti_c54x_cpu *cpu = init_cpu(&fixture, UINT32_C(0xFFFD), storage,
                                    sizeof(storage));
        ti_c54x_state state;
        ti_c54x_run_result result;
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                               &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                               UINT64_C(0xFFFFFFFFFF)) ==
                           TI_C54X_ACCUMULATOR_OK);
        result = ti_c54x_cpu_step(cpu);
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
        TI_C54X_TEST_CHECK(fixture.calls == 2U);
        TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0xFFFD));
        TI_C54X_TEST_CHECK(fixture.addresses[1] == UINT32_C(0xFFFE));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0xFFFF));
        ti_c54x_cpu_deinit(cpu);
    }
    {
        and_cpu_bus_fixture fixture = {
            {shifted_word(0U, 0U, 0U), UINT16_C(1)}, UINT32_C(0xFFFE),
            {0U, 0U}, 0U, false, TI_C54X_BUS_FAULT};
        _Alignas(max_align_t) unsigned char storage[1024];
        ti_c54x_cpu *cpu = init_cpu(&fixture, UINT32_C(0xFFFE), storage,
                                    sizeof(storage));
        ti_c54x_accumulator_state before_accumulators;
        ti_c54x_state state;
        ti_c54x_run_result result;
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                               &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                               UINT64_C(0xFFFFFFFFFF)) ==
                           TI_C54X_ACCUMULATOR_OK);
        before_accumulators = cpu->accumulators;
        result = ti_c54x_cpu_step(cpu);
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
        TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
        TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
        TI_C54X_TEST_CHECK(fixture.calls == 2U);
        TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0xFFFE));
        TI_C54X_TEST_CHECK(fixture.addresses[1] == UINT32_C(0xFFFF));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0xFFFE));
        TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                                  sizeof(before_accumulators)) == 0);
        ti_c54x_cpu_deinit(cpu);
    }
}

int main(void)
{
    static const uint16_t immediate_values[] = {UINT16_C(0), UINT16_C(0xFFFF)};
    uint8_t source;
    uint8_t destination;
    uint8_t shift;
    size_t immediate_index;

    for (source = 0U; source <= 1U; ++source) {
        for (destination = 0U; destination <= 1U; ++destination) {
            for (shift = 0U; shift < UINT8_C(16); ++shift) {
                for (immediate_index = 0U;
                     immediate_index < sizeof(immediate_values) /
                                           sizeof(immediate_values[0]);
                     ++immediate_index) {
                    run_success(shifted_word(source, destination, shift),
                                immediate_values[immediate_index], source,
                                destination, shift, 0U);
                }
            }
            run_success(fixed_word(source, destination), UINT16_C(0x7FFF),
                        source, destination, UINT8_C(16), 0U);
        }
    }
    run_distinct_unknown_destination();
    run_extension_failure();
    run_boundary_fffd_and_fffe();
    return 0;
}
