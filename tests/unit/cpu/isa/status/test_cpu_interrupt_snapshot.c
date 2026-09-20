/* Module contract
 * Owns: public-CPU RED proof for interrupt-state observation
 * (P3-S89, diagnosing the 0xF6A IDLE1 wait): fresh inspect
 * reports reset-cleared IFR with interrupts disabled;
 * SSBX INTM holds known-set; RSBX INTM reports
 * known-clear; IFR stays latched-clear with no arrival
 * source.
 * Does not own: interrupt arrival, take, or wake; status-bit
 * production semantics; shared fixtures; timing; or test
 * manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ST1 INTM is bit 11; status select 1 with the set/clear base. */
#define SSBX_INTM_WORD UINT16_C(0xF7BB)
#define RSBX_INTM_WORD UINT16_C(0xF6BB)

typedef struct snapshot_fixture {
    uint16_t program_word;
} snapshot_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    snapshot_fixture *fixture = (snapshot_fixture *)context;

    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address == UINT32_C(0));
    *value = fixture->program_word;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    (void)context;
    (void)phase;
    (void)transaction_id;
    (void)request;
    (void)observation;

    /* Status-bit ops issue no bus traffic. */
    TI_C54X_TEST_CHECK(false);
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

static void
run_word(uint16_t program_word, ti_c54x_run_result *result,
         ti_c54x_state *view)
{
    snapshot_fixture fixture = {.program_word = program_word};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    const ti_c54x_bus bus = {
        .context = &fixture,
        .program_read_word = read_program,
        .data_endpoint = {.context = &fixture,
                          .advance = advance_data,
                          .cancel = cancel_data}};
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    if (result != NULL) {
        *result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    }
    if (view != NULL) {
        memset(view, 0, sizeof(*view));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, view) ==
                           TI_C54X_ERROR_OK);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fresh_snapshot_unknown(void)
{
    ti_c54x_state view = {0};

    run_word(SSBX_INTM_WORD, NULL, &view);
    /* Reset clears IFR and disables interrupts (INTM=1). */
    TI_C54X_TEST_CHECK(view.ifr_known == true);
    TI_C54X_TEST_CHECK(view.ifr_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(view.intm_known == true);
    TI_C54X_TEST_CHECK(view.intm_value == UINT8_C(1));
}

static void
test_ssbx_intm_known_set(void)
{
    ti_c54x_run_result result;
    ti_c54x_state view = {0};

    run_word(SSBX_INTM_WORD, &result, &view);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(view.intm_known == true);
    TI_C54X_TEST_CHECK(view.intm_value == UINT8_C(1));
    TI_C54X_TEST_CHECK(view.ifr_known == true);
    TI_C54X_TEST_CHECK(view.ifr_value == UINT16_C(0));
}

static void
test_rsbx_intm_known_clear(void)
{
    ti_c54x_run_result result;
    ti_c54x_state view = {0};

    run_word(RSBX_INTM_WORD, &result, &view);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(view.intm_known == true);
    TI_C54X_TEST_CHECK(view.intm_value == UINT8_C(0));
    TI_C54X_TEST_CHECK(view.ifr_known == true);
    TI_C54X_TEST_CHECK(view.ifr_value == UINT16_C(0));
}

int
main(void)
{
    test_fresh_snapshot_unknown();
    test_ssbx_intm_known_set();
    test_rsbx_intm_known_clear();
    return 0;
}
