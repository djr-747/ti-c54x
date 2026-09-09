/* Module contract
 * Owns: CPU-level proofs for OR-immediate two-word fetch, accumulator
 * retirement, exact extension-read order, PC+2 boundaries, and atomic
 * rollback. It covers both shifted and fixed-shift forms.
 * Does not own: catalog/generated tables, pure OR semantics, decoder layout
 * extraction, CPU implementation, timing, repeat/IRQ, memory, diagnostics,
 * or parent test registration.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../../../src/cpu/cpu_internal.h"
#include "../../../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../../../src/state/status/registers.h"
#include "../../../../../../support/test_harness.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct bus_fixture {
    uint16_t first_word;
    uint16_t extension_word;
    ti_c54x_bus_status extension_status;
    ti_c54x_program_word_address expected_first_address;
    ti_c54x_program_word_address expected_extension_address;
    ti_c54x_program_word_address addresses[2];
    unsigned calls;
} bus_fixture;

static ti_c54x_bus_status read_word(void *context,
                                    ti_c54x_program_word_address address,
                                    uint16_t *value)
{
    bus_fixture *fixture = (bus_fixture *)context;

    TI_C54X_TEST_CHECK(fixture->calls < 2U);
    fixture->addresses[fixture->calls] = address;
    if (fixture->calls == 0U) {
        TI_C54X_TEST_CHECK(address == fixture->expected_first_address);
        *value = fixture->first_word;
    } else {
        TI_C54X_TEST_CHECK(address == fixture->expected_extension_address);
        *value = fixture->extension_word;
    }
    fixture->calls++;
    return fixture->calls == 2U ? fixture->extension_status
                                : TI_C54X_BUS_OK;
}

static uint16_t encode_shift(uint8_t source, uint8_t destination,
                             uint8_t shift)
{
    return (uint16_t)(UINT16_C(0xF040) | ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U) | shift);
}

static uint16_t encode_shift16(uint8_t source, uint8_t destination)
{
    return (uint16_t)(UINT16_C(0xF064) | ((uint16_t)source << 9U) |
                      ((uint16_t)destination << 8U));
}

static void seed_cpu(ti_c54x_cpu *cpu, uint64_t accumulator_a,
                     uint64_t accumulator_b)
{
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           accumulator_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           accumulator_b) == TI_C54X_ACCUMULATOR_OK);
    cpu->arch_state.status.st0 = UINT16_C(0xA5C3);
    cpu->arch_state.status.st1 = UINT16_C(0x5A96);
    cpu->arch_state.status.pmst = UINT16_C(0xC35A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
}

static void run_success(uint16_t first_word, uint16_t extension_word,
                        uint8_t source, uint8_t destination, unsigned shift,
                        ti_c54x_program_word_address reset_pc)
{
    bus_fixture fixture = {first_word, extension_word, TI_C54X_BUS_OK,
                           reset_pc, reset_pc + UINT32_C(1), {0U, 0U}, 0U};
    const ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    ti_c54x_status_register_state before_status;
    const uint64_t accumulator_a = UINT64_C(0x0123456789);
    const uint64_t accumulator_b = UINT64_C(0x00ABCDEF01);
    const uint64_t selected = source == 0U ? accumulator_a : accumulator_b;
    const uint64_t expected =
        (selected | ((uint64_t)extension_word << shift)) & TI_C54X_U40_MASK;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    seed_cpu(cpu, accumulator_a, accumulator_b);
    /* The destination is intentionally seeded differently; OR-immediate
     * reads only the selected source and overwrites D. */
    before_status = cpu->arch_state.status;
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.calls == 2U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == reset_pc);
    TI_C54X_TEST_CHECK(fixture.addresses[1] == reset_pc + UINT32_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == reset_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK_U64(destination == 0U ? state.accumulator_a_raw40
                                             : state.accumulator_b_raw40,
                           expected);
    TI_C54X_TEST_CHECK(destination == 0U ? state.accumulator_a_known
                                         : state.accumulator_b_known);
    TI_C54X_TEST_CHECK_U64(destination == 0U ? state.accumulator_b_raw40
                                             : state.accumulator_a_raw40,
                           destination == 0U ? accumulator_b : accumulator_a);
    TI_C54X_TEST_CHECK(destination == 0U ? state.accumulator_b_known
                                         : state.accumulator_a_known);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void test_shifted_and_fixed_forms_fetch_once_and_retire(void)
{
    run_success(encode_shift(UINT8_C(0), UINT8_C(1), UINT8_C(0)),
                UINT16_C(0xFFFF), UINT8_C(0), UINT8_C(1), 0U, UINT32_C(0));
    run_success(encode_shift(UINT8_C(1), UINT8_C(0), UINT8_C(15)),
                UINT16_C(0x7FFF), UINT8_C(1), UINT8_C(0), 15U,
                UINT32_C(0));
    run_success(encode_shift16(UINT8_C(0), UINT8_C(1)), UINT16_C(1),
                UINT8_C(0), UINT8_C(1), 16U, UINT32_C(0));
}

static void test_extension_failure_rolls_back_atomically(void)
{
    bus_fixture fixture = {encode_shift(UINT8_C(0), UINT8_C(1), UINT8_C(1)),
                           UINT16_C(0xCAFE), TI_C54X_BUS_FAULT, 0U, 1U,
                           {0U, 0U}, 0U};
    const ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    const ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state before_state;
    ti_c54x_state after_state;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    seed_cpu(cpu, UINT64_C(0x1234), UINT64_C(0x5678));
    before_status = cpu->arch_state.status;
    before_accumulators = cpu->accumulators;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &before_state) ==
                       TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.calls == 2U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == 0U);
    TI_C54X_TEST_CHECK(fixture.addresses[1] == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &after_state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(after_state.pc_word == before_state.pc_word);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void test_fffd_succeeds_and_fffe_rejects_candidate_atomically(void)
{
    const ti_c54x_program_word_address starts[] = {UINT32_C(0xFFFD),
                                                    UINT32_C(0xFFFE)};
    for (size_t index = 0U; index < sizeof(starts) / sizeof(starts[0]);
         ++index) {
        const ti_c54x_program_word_address start = starts[index];
        bus_fixture fixture = {encode_shift16(UINT8_C(0), UINT8_C(1)),
                               UINT16_C(1), TI_C54X_BUS_OK, start,
                               start + UINT32_C(1), {0U, 0U}, 0U};
        const ti_c54x_bus bus = {.context = &fixture,
                                 .program_read_word = read_word};
        const ti_c54x_config config = {start, {0U, NULL, NULL}};
        _Alignas(max_align_t) unsigned char storage[1024];
        ti_c54x_cpu *cpu = NULL;
        ti_c54x_state before_state;
        ti_c54x_state after_state;
        ti_c54x_accumulator_state before_accumulators;
        ti_c54x_status_register_state before_status;
        ti_c54x_run_result result;

        TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                            &bus, &cpu) == TI_C54X_ERROR_OK);
        seed_cpu(cpu, UINT64_C(0x1234), UINT64_C(0x5678));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &before_state) ==
                           TI_C54X_ERROR_OK);
        before_accumulators = cpu->accumulators;
        before_status = cpu->arch_state.status;
        result = ti_c54x_cpu_step(cpu);
        TI_C54X_TEST_CHECK(fixture.calls == 2U);
        TI_C54X_TEST_CHECK(fixture.addresses[0] == start);
        TI_C54X_TEST_CHECK(fixture.addresses[1] == start + UINT32_C(1));
        if (start == UINT32_C(0xFFFD)) {
            TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
            TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
            TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &after_state) ==
                               TI_C54X_ERROR_OK);
            TI_C54X_TEST_CHECK(after_state.pc_word == UINT32_C(0xFFFF));
        } else {
            TI_C54X_TEST_CHECK(result.reason ==
                               TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
            TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
            TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &after_state) ==
                               TI_C54X_ERROR_OK);
            TI_C54X_TEST_CHECK(after_state.pc_word == before_state.pc_word);
            TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                                      sizeof(before_accumulators)) == 0);
            TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                                      sizeof(before_status)) == 0);
        }
        ti_c54x_cpu_deinit(cpu);
    }
}

int main(void)
{
    test_shifted_and_fixed_forms_fetch_once_and_retire();
    test_extension_failure_rolls_back_atomically();
    test_fffd_succeeds_and_fffe_rejects_candidate_atomically();
    return 0;
}
