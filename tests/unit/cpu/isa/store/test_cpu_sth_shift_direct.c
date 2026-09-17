/* Module contract
 * Owns: public-CPU RED proofs for two-word `STH src[,SHIFT],Smem`
 * direct-DP stores, including signed-SHIFT high-word publication,
 * the TI syntax-4 worked vector, DP gating, and transactional
 * rejection on unknown DP or accumulator input.
 * Does not own: catalog/decode or pure store effects, address-planner
 * internals, endpoint reducer behavior, production code, timing, or
 * diagnostics.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Local two-word program fixture: the shared single-word fixture
 * cannot serve extension words. */

typedef enum shift_direct_memory_mode {
    SHIFT_DIRECT_READY = 0,
    SHIFT_DIRECT_WAIT_THEN_READY = 1,
    SHIFT_DIRECT_FAULT = 2
} shift_direct_memory_mode;

typedef struct shift_direct_fixture {
    uint16_t words[2];
    shift_direct_memory_mode mode;
    unsigned program_calls;
    unsigned endpoint_calls;
    unsigned begin_calls;
    unsigned resume_calls;
    unsigned ready_calls;
    unsigned cancel_calls;
    uint64_t ids[4];
    ti_c54x_bus_word16_request requests[4];
} shift_direct_fixture;

static uint16_t
direct_word1(uint8_t addr7)
{
    return (uint16_t)(UINT16_C(0x6F00) | (uint16_t)(addr7 & UINT8_C(0x7F)));
}

static uint16_t
shift_word2(uint8_t source, int8_t shift)
{
    return (uint16_t)(UINT16_C(0x0C60) |
                      ((uint16_t)(source & UINT8_C(1)) << 8U) |
                      (uint16_t)((uint8_t)shift & UINT8_C(0x1F)));
}

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    shift_direct_fixture *fixture = (shift_direct_fixture *)context;
    const unsigned index = fixture->program_calls;

    TI_C54X_TEST_CHECK(index < 2U);
    TI_C54X_TEST_CHECK(address == (ti_c54x_program_word_address)index);
    TI_C54X_TEST_CHECK(value != NULL);
    *value = fixture->words[index];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    shift_direct_fixture *fixture = (shift_direct_fixture *)context;
    const unsigned index = fixture->endpoint_calls;

    TI_C54X_TEST_CHECK(index < 4U);
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->ids[index] = transaction_id;
    fixture->requests[index] = *request;
    fixture->endpoint_calls++;
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        fixture->begin_calls++;
    } else {
        fixture->resume_calls++;
    }
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .bus_status = TI_C54X_BUS_OK,
        .read_value_word = UINT16_C(0),
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY};
    if (fixture->mode == SHIFT_DIRECT_FAULT) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
    } else if (fixture->mode == SHIFT_DIRECT_WAIT_THEN_READY &&
               fixture->endpoint_calls == 1U) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
    } else {
        fixture->ready_calls++;
    }
}

static ti_c54x_bus_word16_cancel_result
cancel_data(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    shift_direct_fixture *fixture = (shift_direct_fixture *)context;

    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    fixture->cancel_calls++;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *
init_direct(shift_direct_fixture *fixture, unsigned char *storage,
            uint8_t source, uint64_t source_value, uint64_t other_value,
            uint16_t dp_value)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = fixture, .advance = advance_data, .cancel = cancel_data};
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program,
                             .data_endpoint = endpoint};
    const ti_c54x_config config = {0U, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           source == UINT8_C(0) ? TI_C54X_ACCUMULATOR_A
                                                : TI_C54X_ACCUMULATOR_B,
                           source_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators,
                           source == UINT8_C(0) ? TI_C54X_ACCUMULATOR_B
                                                : TI_C54X_ACCUMULATOR_A,
                           other_value) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&cpu->arch_state,
                                                   dp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    return cpu;
}

static void
check_direct_case(uint8_t source, uint8_t addr7, int8_t shift,
                  uint64_t source_value, uint16_t dp_value,
                  uint16_t expected)
{
    shift_direct_fixture fixture = {
        .words = {direct_word1(addr7), shift_word2(source, shift)},
        .mode = SHIFT_DIRECT_READY};
    _Alignas(max_align_t) unsigned char storage[2048] = {0};
    ti_c54x_cpu *cpu =
        init_direct(&fixture, storage, source, source_value,
                    UINT64_C(0), dp_value);
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(2));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.ready_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.requests[0].space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.requests[0].access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture.requests[0].address_word ==
                       (uint16_t)(((uint16_t)dp_value << 7U) | addr7));
    TI_C54X_TEST_CHECK(fixture.requests[0].write_value_word == expected);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_ti_vector(void)
{
    /* TI SPRU172C STH syntax-4 example: STH A,-4 (SHIFT -4) with
     * A FF 8421 1234 under SXM=1 and DP=004 stores F842 at 020A. */
    check_direct_case(UINT8_C(0), UINT8_C(0x0A), -4,
                      UINT64_C(0xFF84211234), UINT16_C(0x004),
                      UINT16_C(0xF842));
    check_direct_case(UINT8_C(0), UINT8_C(0x0A), 0,
                      UINT64_C(0x0012345678), UINT16_C(0x004),
                      UINT16_C(0x1234));
    check_direct_case(UINT8_C(1), UINT8_C(0x0A), 15,
                      UINT64_C(0xFF87654321), UINT16_C(0x004),
                      UINT16_C(0xA190));
}

static void
test_unknown_dp_rejects(void)
{
    shift_direct_fixture fixture = {
        .words = {direct_word1(UINT8_C(0x0A)),
                  shift_word2(UINT8_C(0), -4)},
        .mode = SHIFT_DIRECT_READY};
    _Alignas(max_align_t) unsigned char storage[2048] = {0};
    ti_c54x_cpu *cpu =
        init_direct(&fixture, storage, UINT8_C(0),
                    UINT64_C(0xFF84211234), UINT64_C(0), UINT16_C(0x004));
    ti_c54x_run_result result;

    /* Reset leaves DP unknown: clear its ST0 knownness bits. */
    cpu->arch_state.status.st0_known_mask =
        (uint16_t)(cpu->arch_state.status.st0_known_mask &
                   (uint16_t)~TI_C54X_ST0_DP_MASK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_ti_vector();
    test_unknown_dp_rejects();
    return 0;
}
