/* Module contract
 * Owns: public-CPU RED proof for the CLKMD peripheral cell at
 * DATA 0x58 (P3-S68, TI SPRU131 Figure 8-4): `STM #0x3146`
 * direct retires with zero endpoint calls, mirroring the
 * loader1 prologue word at 0xF07.
 * Does not own: production CPU behavior, catalog/decode or pure
 * effects, address planning, shared fixtures, timing,
 * diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* STM #0x3146,0x58: the loader1 PLL-enable word. */
#define CLKMD_TEST_STM_WORD0 UINT16_C(0x7758)
#define CLKMD_TEST_STM_WORD1 UINT16_C(0x3146)

typedef struct clkmd_fixture {
    uint32_t base;
    uint16_t words[2];
    unsigned word_count;
    unsigned program_calls;
    unsigned data_calls;
} clkmd_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    clkmd_fixture *fixture = (clkmd_fixture *)context;
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
    clkmd_fixture *fixture = (clkmd_fixture *)context;

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
init_cpu(clkmd_fixture *fixture, unsigned char *storage,
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

static void
test_stm_to_clkmd_retires_routed(void)
{
    clkmd_fixture fixture;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    memset(&fixture, 0, sizeof(fixture));
    fixture.base = UINT32_C(0x0100);
    fixture.words[0] = CLKMD_TEST_STM_WORD0;
    fixture.words[1] = CLKMD_TEST_STM_WORD1;
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* MMR-routed: the data endpoint never sees 0x58 traffic. */
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_stm_to_clkmd_retires_routed();
    return 0;
}
