/* Module contract
 * Owns: public SFTL retirement, signed-shift carry/guard semantics, unknown
 * input rollback, non-C status preservation, and program-boundary proof.
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

static uint16_t encode_sftl(uint8_t source, uint8_t destination, int shift)
{
    return (uint16_t)(UINT16_C(0xF0E0) |
                      (uint16_t)((uint16_t)source << 9U) |
                      (uint16_t)((uint16_t)destination << 8U) |
                      (uint16_t)(shift & 0x1F));
}

static uint64_t expected_value(uint64_t source, int shift, uint8_t *out_c)
{
    const uint32_t low = (uint32_t)(source & UINT64_C(0xFFFFFFFF));
    uint32_t result;

    if (shift < 0) {
        const unsigned int amount = (unsigned int)(-shift);
        result = low >> amount;
        *out_c = (uint8_t)((low >> (amount - 1U)) & UINT32_C(1));
    } else if (shift == 0) {
        result = low;
        *out_c = 0U;
    } else {
        const unsigned int amount = (unsigned int)shift;
        result = (uint32_t)(low << amount);
        *out_c = (uint8_t)((low >> (32U - amount)) & UINT32_C(1));
    }
    return (uint64_t)result;
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

static void run_case(uint8_t source, uint8_t destination, int shift,
                     uint64_t a, uint64_t b, uint8_t initial_c,
                     bool initial_c_known)
{
    cpu_fixture fixture = {encode_sftl(source, destination, shift), 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[256];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    ti_c54x_status_register_state before_status;
    const uint64_t selected = source == 0U ? a : b;
    const uint64_t expected_other = destination == 0U ? b : a;
    uint8_t expected_c = 0U;
    const uint64_t expected = expected_value(selected, shift, &expected_c);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A, a) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B, b) ==
                       TI_C54X_ACCUMULATOR_OK);
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
    TI_C54X_TEST_CHECK_U64(destination == 0U ? state.accumulator_a_raw40
                                             : state.accumulator_b_raw40,
                           expected);
    TI_C54X_TEST_CHECK_U64(destination == 0U ? state.accumulator_b_raw40
                                             : state.accumulator_a_raw40,
                           expected_other);
    TI_C54X_TEST_CHECK(state.c_known);
    TI_C54X_TEST_CHECK(state.c == expected_c);
    clear_c_fields(&before_status);
    clear_c_fields(&cpu->status);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->status,
                              sizeof(before_status)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_unknown_selected(uint8_t source)
{
    cpu_fixture fixture = {encode_sftl(source, 0U, 1), 0U};
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
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&cpu->status, 1U) ==
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

static void run_unknown_unselected(void)
{
    cpu_fixture fixture = {encode_sftl(0U, 1U, -1), 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[256];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    uint8_t expected_c = 0U;
    const uint64_t expected = expected_value(UINT64_C(0xABCDE12345), -1,
                                              &expected_c);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0xABCDE12345)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&cpu->status, 0U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK_U64(state.accumulator_b_raw40, expected);
    TI_C54X_TEST_CHECK(state.accumulator_b_known);
    TI_C54X_TEST_CHECK(state.c_known);
    TI_C54X_TEST_CHECK(state.c == expected_c);
    TI_C54X_TEST_CHECK(state.pc_word == 1U);
    ti_c54x_cpu_deinit(cpu);
}

int main(void)
{
    static const int shifts[] = {-16, -1, 0, 1, 15};
    static const uint64_t vectors[] = {
        UINT64_C(0x0000000000), UINT64_C(0x0000000001),
        UINT64_C(0x00007FFFFFFF), UINT64_C(0x000080000000),
        UINT64_C(0x0000FFFFFFFF), UINT64_C(0xFF00000000),
        UINT64_C(0xABCDE12345)};
    size_t shift_index;
    size_t vector_index;
    uint8_t source;
    uint8_t destination;
    uint8_t initial_c;

    for (source = 0U; source <= 1U; ++source) {
        for (destination = 0U; destination <= 1U; ++destination) {
            for (shift_index = 0U;
                 shift_index < sizeof(shifts) / sizeof(shifts[0]);
                 ++shift_index) {
                for (vector_index = 0U;
                     vector_index < sizeof(vectors) / sizeof(vectors[0]);
                     ++vector_index) {
                    for (initial_c = 0U; initial_c <= 1U; ++initial_c) {
                        run_case(source, destination, shifts[shift_index],
                                 source == 0U ? vectors[vector_index]
                                              : UINT64_C(0x13579BDF02),
                                 source == 1U ? vectors[vector_index]
                                              : UINT64_C(0x02468ACE10),
                                 initial_c, true);
                    }
                    run_case(source, destination, shifts[shift_index],
                             source == 0U ? vectors[vector_index]
                                          : UINT64_C(0x13579BDF02),
                             source == 1U ? vectors[vector_index]
                                          : UINT64_C(0x02468ACE10),
                             1U, false);
                }
            }
        }
    }

    run_unknown_selected(0U);
    run_unknown_selected(1U);
    run_unknown_unselected();

    {
        cpu_fixture fixture = {encode_sftl(0U, 1U, 1), UINT32_C(0xFFFF)};
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
        TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&cpu->status, 1U) ==
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
    return 0;
}
