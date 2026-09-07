/* Module contract
 * Owns: CPU-level proofs for XOR two-word immediate fetch ordering,
 * extension-read rollback, PC+2 retirement, source knownness, full-40-bit
 * result formation, status preservation, and the fffd/fffe boundaries.
 * Does not own: CPU implementation, catalog/decode tables, pure handlers,
 * input collection, effect application, timing, repeat/interrupt policy,
 * memory ownership, diagnostics, or shared parent manifests.
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

typedef struct xor_fixture {
    uint16_t first_word;
    uint16_t extension_word;
    ti_c54x_program_word_address first_address;
    ti_c54x_program_word_address extension_address;
    ti_c54x_bus_status extension_status;
    ti_c54x_program_word_address addresses[2];
    unsigned calls;
} xor_fixture;

static ti_c54x_bus_status read_word(void *context,
                                    ti_c54x_program_word_address address,
                                    uint16_t *value)
{
    xor_fixture *fixture = (xor_fixture *)context;

    TI_C54X_TEST_CHECK(fixture->calls < 2U);
    fixture->addresses[fixture->calls] = address;
    if (fixture->calls == 0U) {
        TI_C54X_TEST_CHECK(address == fixture->first_address);
        *value = fixture->first_word;
    } else {
        TI_C54X_TEST_CHECK(address == fixture->extension_address);
        *value = fixture->extension_word;
    }
    fixture->calls++;
    if (fixture->calls == 2U && fixture->extension_status != TI_C54X_BUS_OK) {
        *value = UINT16_C(0xDEAD);
        return fixture->extension_status;
    }
    return TI_C54X_BUS_OK;
}

static uint16_t shifted_word(uint8_t source, uint8_t destination,
                             uint8_t shift)
{
    return (uint16_t)(UINT16_C(0xF050) |
                      ((uint16_t)source << UINT8_C(9)) |
                      ((uint16_t)destination << UINT8_C(8)) |
                      (uint16_t)shift);
}

static uint16_t fixed_word(uint8_t source, uint8_t destination)
{
    return (uint16_t)(UINT16_C(0xF065) |
                      ((uint16_t)source << UINT8_C(9)) |
                      ((uint16_t)destination << UINT8_C(8)));
}

static uint64_t expected_xor(uint64_t source, uint16_t immediate,
                             unsigned shift)
{
    return (source ^ ((uint64_t)immediate << shift)) & TI_C54X_U40_MASK;
}

static void seed_status(ti_c54x_cpu *cpu)
{
    cpu->arch_state.status.st0 = UINT16_C(0xA5C3);
    cpu->arch_state.status.st1 = UINT16_C(0x5A96);
    cpu->arch_state.status.pmst = UINT16_C(0xC35A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
}

static ti_c54x_cpu *init_cpu(xor_fixture *fixture,
                             ti_c54x_program_word_address reset_pc,
                             unsigned char *storage)
{
    const ti_c54x_bus bus = {
        .context = fixture,
        .program_read_word = read_word,
    };
    const ti_c54x_config config = {
        .reset_pc_word = reset_pc,
        .trace = {0U, NULL, NULL},
    };
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, 512U, &config, &bus, &cpu) ==
                       TI_C54X_ERROR_OK);
    return cpu;
}

static void run_success(uint16_t first_word, uint16_t extension_word,
                        uint8_t source, uint8_t destination, unsigned shift,
                        ti_c54x_program_word_address start)
{
    const uint64_t a = UINT64_C(0x123456789A);
    const uint64_t b = UINT64_C(0x00ABCDEF01);
    xor_fixture fixture = {
        .first_word = first_word,
        .extension_word = extension_word,
        .first_address = start,
        .extension_address = start + UINT32_C(1),
        .extension_status = TI_C54X_BUS_OK,
    };
    _Alignas(max_align_t) unsigned char storage[512];
    ti_c54x_cpu *cpu = init_cpu(&fixture, start, storage);
    ti_c54x_status_register_state before_status;
    ti_c54x_state state;
    ti_c54x_run_result result;
    const uint64_t selected = source == 0U ? a : b;
    const uint64_t expected = expected_xor(selected, extension_word, shift);

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A, a) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B, b) ==
                       TI_C54X_ACCUMULATOR_OK);
    seed_status(cpu);
    before_status = cpu->arch_state.status;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == first_word);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == start + UINT32_C(2));
    TI_C54X_TEST_CHECK(fixture.calls == 2U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == start);
    TI_C54X_TEST_CHECK(fixture.addresses[1] == start + UINT32_C(1));
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK_U64(cpu->accumulators.a_raw40,
                           destination == 0U ? expected : a);
    TI_C54X_TEST_CHECK_U64(cpu->accumulators.b_raw40,
                           destination == 1U ? expected : b);
    TI_C54X_TEST_CHECK(destination == 0U ? cpu->accumulators.known_mask & 1U
                                         : cpu->accumulators.known_mask & 2U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == start + UINT32_C(2));
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == UINT64_C(1));
    TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
    ti_c54x_cpu_deinit(cpu);
}

static void test_matrix_and_pc_plus_two(void)
{
    run_success(shifted_word(0U, 0U, 0U), UINT16_C(0x0000), 0U, 0U, 0U,
                UINT32_C(0));
    run_success(shifted_word(0U, 1U, 1U), UINT16_C(0x0001), 0U, 1U, 1U,
                UINT32_C(0));
    run_success(shifted_word(1U, 0U, 15U), UINT16_C(0x7FFF), 1U, 0U, 15U,
                UINT32_C(0));
    run_success(fixed_word(1U, 1U), UINT16_C(0xFFFF), 1U, 1U, 16U,
                UINT32_C(0));
}

static void test_extension_failure_is_atomic(void)
{
    const uint16_t first_word = shifted_word(0U, 1U, 7U);
    xor_fixture fixture = {
        .first_word = first_word,
        .extension_word = UINT16_C(0xBEEF),
        .first_address = UINT32_C(0),
        .extension_address = UINT32_C(1),
        .extension_status = TI_C54X_BUS_FAULT,
    };
    _Alignas(max_align_t) unsigned char storage[512];
    ti_c54x_cpu *cpu = init_cpu(&fixture, 0U, storage);
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_state state;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x123456789A)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x00ABCDEF01)) == TI_C54X_ACCUMULATOR_OK);
    seed_status(cpu);
    before_status = cpu->arch_state.status;
    before_accumulators = cpu->accumulators;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_FAULT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(fixture.calls == 2U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0));
    TI_C54X_TEST_CHECK(fixture.addresses[1] == UINT32_C(1));
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(0));
    ti_c54x_cpu_deinit(cpu);
}

static void test_fffd_two_word_instruction_retires_at_ffff(void)
{
    run_success(fixed_word(0U, 1U), UINT16_C(0x0001), 0U, 1U, 16U,
                UINT32_C(0xFFFD));
}

static void test_fffe_fetches_extension_then_rejects_candidate_pc(void)
{
    const uint16_t first_word = shifted_word(0U, 1U, 1U);
    xor_fixture fixture = {
        .first_word = first_word,
        .extension_word = UINT16_C(0x0001),
        .first_address = UINT32_C(0xFFFE),
        .extension_address = UINT32_C(0xFFFF),
        .extension_status = TI_C54X_BUS_OK,
    };
    _Alignas(max_align_t) unsigned char storage[512];
    ti_c54x_cpu *cpu = init_cpu(&fixture, UINT32_C(0xFFFE), storage);
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x123456789A)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x00ABCDEF01)) == TI_C54X_ACCUMULATOR_OK);
    seed_status(cpu);
    before_status = cpu->arch_state.status;
    before_accumulators = cpu->accumulators;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason ==
                       TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0xFFFE));
    TI_C54X_TEST_CHECK(fixture.calls == 2U);
    TI_C54X_TEST_CHECK(fixture.addresses[0] == UINT32_C(0xFFFE));
    TI_C54X_TEST_CHECK(fixture.addresses[1] == UINT32_C(0xFFFF));
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void test_unknown_selected_source_rolls_back(void)
{
    const uint16_t first_word = shifted_word(0U, 1U, 0U);
    xor_fixture fixture = {
        .first_word = first_word,
        .extension_word = UINT16_C(0x0001),
        .first_address = UINT32_C(0),
        .extension_address = UINT32_C(1),
        .extension_status = TI_C54X_BUS_OK,
    };
    _Alignas(max_align_t) unsigned char storage[512];
    ti_c54x_cpu *cpu = init_cpu(&fixture, 0U, storage);
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x00ABCDEF01)) == TI_C54X_ACCUMULATOR_OK);
    seed_status(cpu);
    before_status = cpu->arch_state.status;
    before_accumulators = cpu->accumulators;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0));
    TI_C54X_TEST_CHECK(fixture.calls == 2U);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

int main(void)
{
    test_matrix_and_pc_plus_two();
    test_extension_failure_is_atomic();
    test_fffd_two_word_instruction_retires_at_ffff();
    test_fffe_fetches_extension_then_rejects_candidate_pc();
    test_unknown_selected_source_rolls_back();
    return 0;
}
