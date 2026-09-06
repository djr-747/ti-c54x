/* Module contract
 * Owns: the CPU-level RED proof for the AND/OR/XOR register-only accumulator
 * families, including retirement, full-40-bit operands, status preservation,
 * knownness rollback, same-lane admission, and the program boundary.
 * Does not own: CPU implementation, catalog/decode tables, pure handlers,
 * input collection, effect application, timing, or generated artifacts.
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

static uint16_t encode_logical(uint16_t base, uint8_t source,
                               uint8_t destination, uint8_t raw_shift)
{
    return (uint16_t)(base | (uint16_t)((uint16_t)source << 9U) |
                      (uint16_t)((uint16_t)destination << 8U) |
                      (uint16_t)raw_shift);
}

static int signed_shift(uint8_t raw_shift)
{
    return raw_shift < 16U ? (int)raw_shift : (int)raw_shift - 32;
}

static uint64_t shifted_source(uint64_t source, int shift)
{
    const uint64_t mask = UINT64_C(0xFFFFFFFFFF);

    source &= mask;
    if (shift < 0) {
        return source >> (unsigned int)(-shift);
    }
    if (shift == 0) {
        return source;
    }
    return (source << (unsigned int)shift) & mask;
}

static uint64_t logical_expected(uint16_t base, uint64_t source,
                                 uint64_t destination, int shift)
{
    const uint64_t shifted = shifted_source(source, shift);

    switch (base) {
    case UINT16_C(0xF080):
        return destination & shifted;
    case UINT16_C(0xF0A0):
        return destination | shifted;
    case UINT16_C(0xF0C0):
        return destination ^ shifted;
    default:
        TI_C54X_TEST_CHECK(false);
        return 0U;
    }
}

static void seed_status(ti_c54x_status_register_state *status)
{
    /* Directly seed every private status carrier so a successful logical
     * retirement proves byte-for-byte preservation, including knownness. */
    status->st0 = UINT16_C(0xA5C3);
    status->st1 = UINT16_C(0x5A96);
    status->pmst = UINT16_C(0xC35A);
    status->st0_known_mask = UINT16_C(0xFFFF);
    status->st1_known_mask = UINT16_C(0xFFFF);
    status->pmst_known_mask = UINT16_C(0xFFFF);
}

static void run_success(uint16_t base, uint8_t source, uint8_t destination,
                        uint8_t raw_shift, uint64_t a, uint64_t b)
{
    cpu_fixture fixture = {
        encode_logical(base, source, destination, raw_shift), 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[256];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    ti_c54x_status_register_state before_status;
    const uint64_t source_value = source == 0U ? a : b;
    const uint64_t destination_value = destination == 0U ? a : b;
    const uint64_t expected = logical_expected(
        base, source_value, destination_value, signed_shift(raw_shift));

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A, a) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B, b) ==
                       TI_C54X_ACCUMULATOR_OK);
    seed_status(&cpu->status);
    before_status = cpu->status;

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
    TI_C54X_TEST_CHECK_U64(destination == 0U ? state.accumulator_b_raw40
                                             : state.accumulator_a_raw40,
                           destination == 0U ? b : a);
    TI_C54X_TEST_CHECK(destination == 0U ? state.accumulator_a_known
                                         : state.accumulator_b_known);
    TI_C54X_TEST_CHECK(state.c_known);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->status,
                              sizeof(before_status)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_same_lane_with_unselected_unknown(uint16_t base,
                                                   uint8_t lane,
                                                   uint8_t raw_shift)
{
    const uint64_t value = UINT64_C(0xF123456789);
    const uint16_t word = encode_logical(base, lane, lane, raw_shift);
    cpu_fixture fixture = {word, 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[256];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    ti_c54x_status_register_state before_status;
    const uint64_t expected = logical_expected(base, value, value,
                                               signed_shift(raw_shift));

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           lane == 0U ? TI_C54X_ACCUMULATOR_A
                                      : TI_C54X_ACCUMULATOR_B,
                           value) == TI_C54X_ACCUMULATOR_OK);
    seed_status(&cpu->status);
    /* Logical operations do not consume C.  An unknown C must be copied,
     * not rejected or synthesized, while the accumulator lane is retired. */
    cpu->status.st0_known_mask = (uint16_t)(
        cpu->status.st0_known_mask & (uint16_t)~TI_C54X_ST0_C_MASK);
    before_status = cpu->status;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == 1U);
    TI_C54X_TEST_CHECK_U64(lane == 0U ? state.accumulator_a_raw40
                                      : state.accumulator_b_raw40,
                           expected);
    TI_C54X_TEST_CHECK(lane == 0U ? !state.accumulator_b_known
                                  : !state.accumulator_a_known);
    TI_C54X_TEST_CHECK(!state.c_known);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->status,
                              sizeof(before_status)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_invalid_participating(uint16_t word, uint8_t noncanonical_lane)
{
    cpu_fixture fixture = {word, 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[256];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_state state;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    seed_status(&cpu->status);
    if (noncanonical_lane <= 1U) {
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                               &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                               UINT64_C(0x0123456789)) ==
                           TI_C54X_ACCUMULATOR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                               &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                               UINT64_C(0x0123456789)) ==
                           TI_C54X_ACCUMULATOR_OK);
        if (noncanonical_lane == 0U) {
            cpu->accumulators.a_raw40 = UINT64_C(0x10000000000);
        } else {
            cpu->accumulators.b_raw40 = UINT64_C(0x10000000000);
        }
        cpu->accumulators.known_mask = UINT8_C(3);
    } else {
        TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                               &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                               UINT64_C(0x0123456789)) ==
                           TI_C54X_ACCUMULATOR_OK);
    }
    before_status = cpu->status;
    before_accumulators = cpu->accumulators;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(cpu->pc_word == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 0U);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_boundary(void)
{
    const uint16_t word = encode_logical(UINT16_C(0xF080), 0U, 1U, 1U);
    cpu_fixture fixture = {word, UINT32_C(0xFFFF)};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {UINT32_C(0xFFFF), {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[256];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_state state;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0xF123456789)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x00ABCDEF12)) == TI_C54X_ACCUMULATOR_OK);
    seed_status(&cpu->status);
    before_status = cpu->status;
    before_accumulators = cpu->accumulators;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason ==
                       TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(cpu->pc_word == UINT32_C(0xFFFF));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 0U);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

int main(void)
{
    static const uint16_t bases[] = {
        UINT16_C(0xF080), UINT16_C(0xF0A0), UINT16_C(0xF0C0)};
    static const uint64_t vectors[][2] = {
        {UINT64_C(0xF123456789), UINT64_C(0x00ABCDEF12)},
        {UINT64_C(0x8000000001), UINT64_C(0x7F00000002)}};
    size_t base_index;
    size_t vector_index;
    uint8_t source;
    uint8_t destination;
    uint8_t raw_shift;

    /* Every op family, S/D combination, and five-bit shift encoding (384
     * encodings) reaches the same generic CPU retirement proof. */
    for (base_index = 0U; base_index < sizeof(bases) / sizeof(bases[0]);
         ++base_index) {
        for (source = 0U; source <= 1U; ++source) {
            for (destination = 0U; destination <= 1U; ++destination) {
                for (raw_shift = 0U; raw_shift < 32U; ++raw_shift) {
                    for (vector_index = 0U;
                         vector_index < sizeof(vectors) / sizeof(vectors[0]);
                         ++vector_index) {
                        run_success(bases[base_index], source, destination,
                                    raw_shift, vectors[vector_index][0],
                                    vectors[vector_index][1]);
                    }
                }
            }
        }
    }

    run_same_lane_with_unselected_unknown(UINT16_C(0xF080), 0U, 16U);
    run_same_lane_with_unselected_unknown(UINT16_C(0xF0A0), 1U, 31U);
    run_same_lane_with_unselected_unknown(UINT16_C(0xF0C0), 0U, 0U);

    /* S!=D requires both participating lanes to be known and canonical. */
    run_invalid_participating(encode_logical(UINT16_C(0xF080), 0U, 1U, 1U),
                              UINT8_C(2));
    run_invalid_participating(encode_logical(UINT16_C(0xF0A0), 1U, 0U, 15U),
                              UINT8_C(2));
    run_invalid_participating(encode_logical(UINT16_C(0xF0C0), 0U, 1U, 31U),
                              UINT8_C(0));

    /* A noncanonical participating source is invalid in either orientation;
     * so is a noncanonical distinct destination. */
    run_invalid_participating(encode_logical(UINT16_C(0xF080), 0U, 1U, 1U),
                              UINT8_C(0));
    run_invalid_participating(encode_logical(UINT16_C(0xF0A0), 1U, 0U, 15U),
                              UINT8_C(1));
    run_invalid_participating(encode_logical(UINT16_C(0xF0C0), 0U, 1U, 31U),
                              UINT8_C(1));
    run_invalid_participating(encode_logical(UINT16_C(0xF080), 1U, 0U, 1U),
                              UINT8_C(0));
    run_boundary();
    return 0;
}
