/* Module contract
 * Owns: public-CPU RED proofs for the SWWSR peripheral cell at
 * DATA 0x28 (P3-S67, TI SPRU131 Tables 8-2/10-2): `STM #lk`
 * direct retires with zero endpoint calls, `LDM 0x28,A`
 * reads back the written value, and a fresh reset reads the
 * documented 0x7FFF reset value.
 * Does not own: production CPU behavior, catalog/decode or pure
 * effects, address planning, shared fixtures, timing,
 * diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* STM #0x2000,0x28 then LDM 0x28,A: mirrors the loader1
 * prologue pair at 0xF05. */
#define SWWSR_TEST_STM_WORD0 UINT16_C(0x7728)
#define SWWSR_TEST_STM_WORD1 UINT16_C(0x2000)
#define SWWSR_TEST_LDM_WORD UINT16_C(0x4828)

typedef struct swwsr_fixture {
    uint32_t base;
    uint16_t words[4];
    unsigned word_count;
    unsigned program_calls;
    unsigned data_calls;
} swwsr_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    swwsr_fixture *fixture = (swwsr_fixture *)context;
    uint32_t offset;

    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address >= fixture->base);
    offset = address - fixture->base;
    TI_C54X_TEST_CHECK(offset < fixture->word_count);
    *value = fixture->words[offset];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    swwsr_fixture *fixture = (swwsr_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->data_calls++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
}

static ti_c54x_bus_word16_cancel_result
cancel_data(void *context, uint64_t transaction_id,
            const ti_c54x_bus_word16_request *request)
{
    (void)context;
    (void)transaction_id;
    (void)request;
    return TI_C54X_BUS_WORD16_CANCEL_RESULT_CANCELLED;
}

static ti_c54x_cpu *
init_cpu(swwsr_fixture *fixture, unsigned char *storage,
         uint32_t reset_pc)
{
    const ti_c54x_bus bus = {
        .context = fixture,
        .program_read_word = read_program,
        .data_endpoint = {.context = fixture,
                          .advance = advance_data,
                          .cancel = cancel_data}};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

static uint64_t
read_a_raw40(const ti_c54x_cpu *cpu)
{
    ti_c54x_accumulator_snapshot snapshot;

    /* The LDM leg touches A only; B stays unknown, so the
     * whole-snapshot status is UNKNOWN by contract. */
    (void)ti_c54x_accumulator_read_snapshot(&cpu->accumulators,
                                            &snapshot);
    TI_C54X_TEST_CHECK(snapshot.a_known);
    return snapshot.a_raw40;
}

static void
test_stm_ldm_round_trip(void)
{
    swwsr_fixture fixture;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    memset(&fixture, 0, sizeof(fixture));
    fixture.base = UINT32_C(0x0100);
    fixture.words[0] = SWWSR_TEST_STM_WORD0;
    fixture.words[1] = SWWSR_TEST_STM_WORD1;
    fixture.words[2] = SWWSR_TEST_LDM_WORD;
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    /* MMR-routed: the data endpoint never sees 0x28 traffic. */
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK_U64(read_a_raw40(cpu), UINT64_C(0x2000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_reset_value_reads_pinned(void)
{
    swwsr_fixture fixture;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    memset(&fixture, 0, sizeof(fixture));
    fixture.base = UINT32_C(0x0100);
    fixture.words[0] = SWWSR_TEST_LDM_WORD;
    fixture.word_count = 1U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK_U64(read_a_raw40(cpu), UINT64_C(0x7FFF));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_stm_ldm_round_trip();
    test_reset_value_reads_pinned();
    return 0;
}
