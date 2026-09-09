/* Module contract
 * Owns: public SFTC retirement, accumulator-source selection, TC result
 * staging, C preservation, unknown-input rollback, and program-boundary
 * proof.
 * Does not own: CPU implementation, decode tables, opcode handlers, timing,
 * or input-collection implementation.
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

static uint16_t encode_sftc(uint8_t source)
{
    return (uint16_t)(UINT16_C(0xF494) |
                      (uint16_t)((uint16_t)source << 8U));
}

static uint64_t expected_value(uint64_t source)
{
    const uint64_t low32 = source & UINT64_C(0xFFFFFFFF);

    if (low32 == UINT64_C(0) ||
        (((low32 >> 31U) ^ (low32 >> 30U)) & UINT64_C(1)) != UINT64_C(0)) {
        return source;
    }
    return (source & UINT64_C(0xFF00000000)) |
           ((low32 << 1U) & UINT64_C(0xFFFFFFFF));
}

static uint8_t expected_tc(uint64_t source)
{
    const uint64_t low32 = source & UINT64_C(0xFFFFFFFF);

    return low32 == UINT64_C(0) ||
                   (((low32 >> 31U) ^ (low32 >> 30U)) & UINT64_C(1)) !=
                       UINT64_C(0)
               ? UINT8_C(1)
               : UINT8_C(0);
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

static void clear_tc_fields(ti_c54x_status_register_state *status)
{
    status->st0 = (uint16_t)(status->st0 & (uint16_t)~TI_C54X_ST0_TC_MASK);
    status->st0_known_mask = (uint16_t)(
        status->st0_known_mask & (uint16_t)~TI_C54X_ST0_TC_MASK);
}

static void run_case(uint8_t source, uint64_t a, uint64_t b, uint8_t initial_c,
                     bool initial_c_known, uint8_t initial_tc,
                     bool initial_tc_known)
{
    cpu_fixture fixture = {encode_sftc(source), 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    ti_c54x_status_register_state before_status;
    const uint64_t selected = source == UINT8_C(0) ? a : b;
    const uint64_t other = source == UINT8_C(0) ? b : a;
    ti_c54x_status_register_field_result c_result;
    ti_c54x_status_register_field_result tc_result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A, a) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B, b) ==
                       TI_C54X_ACCUMULATOR_OK);
    seed_non_c_status(&cpu->arch_state.status);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&cpu->arch_state.status,
                                                       initial_c) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(&cpu->arch_state.status,
                                                        initial_tc) ==
                       TI_C54X_STATUS_REGISTER_OK);
    if (!initial_c_known) {
        cpu->arch_state.status.st0_known_mask = (uint16_t)(
            cpu->arch_state.status.st0_known_mask & (uint16_t)~TI_C54X_ST0_C_MASK);
    }
    if (!initial_tc_known) {
        cpu->arch_state.status.st0_known_mask = (uint16_t)(
            cpu->arch_state.status.st0_known_mask & (uint16_t)~TI_C54X_ST0_TC_MASK);
    }
    before_status = cpu->arch_state.status;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.pc_word == 1U);
    TI_C54X_TEST_CHECK(state.instructions_attempted_total == 1U);
    TI_C54X_TEST_CHECK(state.instructions_retired_total == 1U);
    TI_C54X_TEST_CHECK_U64(source == UINT8_C(0) ? state.accumulator_a_raw40
                                                : state.accumulator_b_raw40,
                           expected_value(selected));
    TI_C54X_TEST_CHECK(state.accumulator_a_known);
    TI_C54X_TEST_CHECK(state.accumulator_b_known);
    TI_C54X_TEST_CHECK_U64(source == UINT8_C(0) ? state.accumulator_b_raw40
                                                : state.accumulator_a_raw40,
                           other);

    c_result = ti_c54x_status_register_read_c(&cpu->arch_state.status);
    tc_result = ti_c54x_status_register_read_tc(&cpu->arch_state.status);
    TI_C54X_TEST_CHECK(c_result.status == TI_C54X_STATUS_REGISTER_UNKNOWN ||
                       c_result.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(c_result.known == initial_c_known);
    if (initial_c_known) {
        TI_C54X_TEST_CHECK(c_result.raw_value == initial_c);
    }
    TI_C54X_TEST_CHECK(tc_result.status == TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(tc_result.known);
    TI_C54X_TEST_CHECK(tc_result.raw_value == expected_tc(selected));

    clear_c_fields(&before_status);
    clear_c_fields(&cpu->arch_state.status);
    clear_tc_fields(&before_status);
    clear_tc_fields(&cpu->arch_state.status);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_unknown_selected(uint8_t source)
{
    cpu_fixture fixture = {encode_sftc(source), 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_status_register_state before_status;
    ti_c54x_accumulator_state before_accumulators;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           source == UINT8_C(0) ? TI_C54X_ACCUMULATOR_B
                                                : TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x123456789A)) == TI_C54X_ACCUMULATOR_OK);
    seed_non_c_status(&cpu->arch_state.status);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&cpu->arch_state.status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(&cpu->arch_state.status, 0U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    cpu->arch_state.status.st0_known_mask = (uint16_t)(
        cpu->arch_state.status.st0_known_mask & (uint16_t)~TI_C54X_ST0_TC_MASK);
    before_status = cpu->arch_state.status;
    before_accumulators = cpu->accumulators;

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == 0U);
    TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                              sizeof(before_status)) == 0);
    TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                              sizeof(before_accumulators)) == 0);
    ti_c54x_cpu_deinit(cpu);
}

static void run_unknown_unselected_allowed(uint8_t source)
{
    cpu_fixture fixture = {encode_sftc(source), 0U};
    ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
    ti_c54x_config config = {0U, {0U, NULL, NULL}};
    _Alignas(max_align_t) unsigned char storage[1024];
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state state;
    ti_c54x_run_result result;
    const uint64_t selected = UINT64_C(0xFFFFFFF001);

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, sizeof(storage), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           source == UINT8_C(0) ? TI_C54X_ACCUMULATOR_A
                                                : TI_C54X_ACCUMULATOR_B,
                           selected) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(&cpu->arch_state.status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(&cpu->arch_state.status, 1U) ==
                       TI_C54X_STATUS_REGISTER_OK);
    cpu->arch_state.status.st0_known_mask = (uint16_t)(
        cpu->arch_state.status.st0_known_mask & (uint16_t)~TI_C54X_ST0_TC_MASK);

    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
    TI_C54X_TEST_CHECK(result.instructions_retired == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK_U64(source == UINT8_C(0) ? state.accumulator_a_raw40
                                                : state.accumulator_b_raw40,
                           UINT64_C(0xFFFFFFE002));
    TI_C54X_TEST_CHECK(source == UINT8_C(0) ? !state.accumulator_b_known
                                            : !state.accumulator_a_known);
    TI_C54X_TEST_CHECK(state.pc_word == 1U);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_tc(&cpu->arch_state.status).known);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_read_tc(&cpu->arch_state.status).raw_value ==
                       0U);
    ti_c54x_cpu_deinit(cpu);
}

int main(void)
{
    static const uint64_t vectors[] = {
        UINT64_C(0x0000000000), UINT64_C(0x0000000001),
        UINT64_C(0x00003FFFFFFF), UINT64_C(0x0000C0000001),
        UINT64_C(0x000080000000), UINT64_C(0x000040000000),
        UINT64_C(0xFFFFFFF001), UINT64_C(0x1200000001)};
    size_t index;
    uint8_t source;
    uint8_t initial_c;
    uint8_t initial_tc;
    unsigned c_known_index;
    unsigned tc_known_index;

    for (source = UINT8_C(0); source <= UINT8_C(1); ++source) {
        for (index = 0U; index < sizeof(vectors) / sizeof(vectors[0]);
             ++index) {
            for (initial_c = UINT8_C(0); initial_c <= UINT8_C(1);
                 ++initial_c) {
                for (initial_tc = UINT8_C(0); initial_tc <= UINT8_C(1);
                     ++initial_tc) {
                    for (c_known_index = 0U; c_known_index <= 1U;
                         ++c_known_index) {
                        for (tc_known_index = 0U; tc_known_index <= 1U;
                             ++tc_known_index) {
                            run_case(source,
                                     source == UINT8_C(0) ? vectors[index]
                                                          : UINT64_C(0x13579BDF02),
                                     source == UINT8_C(1) ? vectors[index]
                                                          : UINT64_C(0x02468ACE10),
                                     initial_c, c_known_index != 0U,
                                     initial_tc, tc_known_index != 0U);
                        }
                    }
                }
            }
        }
    }

    run_unknown_selected(UINT8_C(0));
    run_unknown_selected(UINT8_C(1));
    run_unknown_unselected_allowed(UINT8_C(0));
    run_unknown_unselected_allowed(UINT8_C(1));

    {
        cpu_fixture fixture = {encode_sftc(UINT8_C(0)), UINT32_C(0xFFFF)};
        ti_c54x_bus bus = {.context = &fixture, .program_read_word = read_word};
        ti_c54x_config config = {UINT32_C(0xFFFF), {0U, NULL, NULL}};
        _Alignas(max_align_t) unsigned char storage[1024];
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
        before_status = cpu->arch_state.status;
        before_accumulators = cpu->accumulators;
        result = ti_c54x_cpu_step(cpu);
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_UNRESOLVED_PROGRAM_BOUNDARY);
        TI_C54X_TEST_CHECK(result.instructions_attempted == 1U);
        TI_C54X_TEST_CHECK(result.instructions_retired == 0U);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0xFFFF));
        TI_C54X_TEST_CHECK(memcmp(&before_status, &cpu->arch_state.status,
                                  sizeof(before_status)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&before_accumulators, &cpu->accumulators,
                                  sizeof(before_accumulators)) == 0);
        ti_c54x_cpu_deinit(cpu);
    }
    return 0;
}
