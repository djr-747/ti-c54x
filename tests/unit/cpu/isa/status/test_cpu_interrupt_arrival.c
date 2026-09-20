/* Module contract
 * Owns: public-CPU RED proof for external maskable interrupt receive
 * (P3-S93, TI SPRU131G section 6.10 phase 1: a maskable request sets
 * its IFR bit on receipt): a fresh CPU latches one requested bit
 * visible via inspect with INTM undisturbed; a second requested bit
 * accumulates; null CPU, zero, and multi-bit masks reject with
 * INVALID_ARGUMENT and mutate nothing. No run, arbitration, take,
 * or wake is involved.
 * Does not own: interrupt acknowledgment, take, or wake; status-bit
 * production semantics; shared fixtures; timing; or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* C542 IFR (TI SPRU131G Figure 6-2(b)): bit 1 is INT1, bit 2 INT2. */
#define ARRIVAL_INT1_MASK UINT16_C(0x0002)
#define ARRIVAL_INT2_MASK UINT16_C(0x0004)

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    (void)context;
    (void)address;
    (void)value;

    return TI_C54X_BUS_UNMAPPED;
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

    /* Arrival needs no run and issues no bus traffic. */
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
init_cpu(unsigned char *storage, ti_c54x_cpu **cpu)
{
    const ti_c54x_bus bus = {
        .context = NULL,
        .program_read_word = read_program,
        .data_endpoint = {.context = NULL,
                          .advance = advance_data,
                          .cancel = cancel_data}};
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(),
                                        &config, &bus,
                                        cpu) == TI_C54X_ERROR_OK);
}

static void
inspect_view(ti_c54x_cpu *cpu, ti_c54x_state *view)
{
    memset(view, 0, sizeof(*view));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, view) ==
                       TI_C54X_ERROR_OK);
}

static void
test_arrival_latches_int1(void)
{
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state view = {0};

    init_cpu(storage, &cpu);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt(cpu,
                                                    ARRIVAL_INT1_MASK) ==
                       TI_C54X_ERROR_OK);
    inspect_view(cpu, &view);
    TI_C54X_TEST_CHECK(view.ifr_known == true);
    TI_C54X_TEST_CHECK(view.ifr_value == ARRIVAL_INT1_MASK);
    TI_C54X_TEST_CHECK(view.intm_known == true);
    TI_C54X_TEST_CHECK(view.intm_value == UINT8_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_arrival_accumulates(void)
{
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state view = {0};

    init_cpu(storage, &cpu);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt(cpu,
                                                    ARRIVAL_INT1_MASK) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt(cpu,
                                                    ARRIVAL_INT2_MASK) ==
                       TI_C54X_ERROR_OK);
    inspect_view(cpu, &view);
    TI_C54X_TEST_CHECK(view.ifr_known == true);
    TI_C54X_TEST_CHECK(view.ifr_value ==
                       (uint16_t)(ARRIVAL_INT1_MASK | ARRIVAL_INT2_MASK));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_arrival_rejects(void)
{
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_state view = {0};

    init_cpu(storage, &cpu);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt(NULL,
                                                    ARRIVAL_INT1_MASK) ==
                       TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt(cpu,
                                                    UINT16_C(0)) ==
                       TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt(
                           cpu,
                           (uint16_t)(ARRIVAL_INT1_MASK |
                                      ARRIVAL_INT2_MASK)) ==
                       TI_C54X_ERROR_INVALID_ARGUMENT);
    inspect_view(cpu, &view);
    TI_C54X_TEST_CHECK(view.ifr_known == true);
    TI_C54X_TEST_CHECK(view.ifr_value == UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_arrival_latches_int1();
    test_arrival_accumulates();
    test_arrival_rejects();
    return 0;
}
