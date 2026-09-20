/* Module contract
 * Owns: public-CPU RED proof for IDLE K (P3-S88, TI
 * SPRU172C power-down, 1 word): the loader1 word at
 * 0xF6A stops the run with TI_C54X_STOP_IDLE —
 * attempted, unretired, pc held, no bus traffic.
 * Does not own: production CPU behavior, catalog/decode
 * or pure effects, address planning, shared fixtures,
 * timing, diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../src/cpu/cpu_internal.h"
#include "../../../src/state/register_layout.h"
#include "../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    (void)context;

    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address == UINT32_C(0x0100));
    *value = UINT16_C(0xF4E1);
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

    /* IDLE issues no bus traffic. */
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
test_idle_stops(void)
{
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    const ti_c54x_bus bus = {
        .context = NULL,
        .program_read_word = read_program,
        .data_endpoint = {.context = NULL,
                          .advance = advance_data,
                          .cancel = cancel_data}};
    const ti_c54x_config config = {UINT32_C(0x0100), {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(10));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_IDLE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0100));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_idle_stops();
    return 0;
}
