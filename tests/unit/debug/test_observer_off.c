/* Module contract
 * Owns: the observer-disabled build smoke test.
 * Does not own: observer implementation or diagnostic policy. The target is
 * compiled directly with TI_C54X_ENABLE_OBSERVERS=0 to prove compile-out.
 */

#include <stdint.h>
#include <stdlib.h>

#include <ti_c54x/cpu.h>

#include "../../support/test_harness.h"

static ti_c54x_bus_status
read_program(void *context,
             ti_c54x_program_word_address address_word,
             uint16_t *value_word)
{
    (void)context;
    (void)address_word;
    *value_word = UINT16_C(0x1000);
    return TI_C54X_BUS_OK;
}

static void
unexpected_event(void *context, const ti_c54x_trace_event *event)
{
    (void)context;
    (void)event;
    TI_C54X_TEST_CHECK(false);
}

int
main(void)
{
    ti_c54x_bus bus = {0};
    ti_c54x_config config = {0U, {TI_C54X_TRACE_MASK_ALL, NULL,
                                  unexpected_event}};
    void *storage = malloc(ti_c54x_cpu_size());
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(storage != NULL);
    bus.program_read_word = read_program;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_step(cpu);
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_UNIMPLEMENTED_OPCODE);
    ti_c54x_cpu_deinit(cpu);
    free(storage);
    return 0;
}
