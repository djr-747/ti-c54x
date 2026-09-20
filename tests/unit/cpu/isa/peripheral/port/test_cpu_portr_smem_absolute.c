/* Module contract
 * Owns: public-CPU RED proof for absolute PORTR *(abs),PA
 * (P3-S87, TI SPRU172C §4 p. 4-129, 3 words):
 * the loader1 words at 0xF97 retire `PORTR *(0x0008),0x2D`,
 * reading the endpoint IO port and writing it to the
 * endpoint DATA word with no status change.
 * Does not own: production CPU behavior, catalog/decode or pure
 * effects, address planning, shared fixtures, timing,
 * diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../../src/cpu/cpu_internal.h"
#include "../../../../../../src/state/register_layout.h"
#include "../../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PORTR_ABS_WORD0 UINT16_C(0x74F8)
#define PORTR_ABS_ADDR UINT16_C(0x0008)
#define PORTR_ABS_PORT_DATA UINT16_C(0x1234)
#define PORTR_ABS_PA UINT16_C(0x002D)

typedef struct portr_abs_fixture {
    uint32_t base;
    uint16_t words[3];
    unsigned word_count;
    unsigned io_reads;
    unsigned data_writes;
    uint32_t last_data_address;
    uint16_t last_data_value;
} portr_abs_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    portr_abs_fixture *fixture = (portr_abs_fixture *)context;
    uint32_t offset;

    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address >= fixture->base);
    offset = address - fixture->base;
    TI_C54X_TEST_CHECK(offset < fixture->word_count);
    *value = fixture->words[offset];
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    portr_abs_fixture *fixture = (portr_abs_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    /* First leg: the IO port read (PORTR pair precedent:
     * both legs ride the data endpoint). */
    if (request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_IO &&
        request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
        request->address_word == UINT32_C(0x002D)) {
        fixture->io_reads++;
        observation->read_value_word = PORTR_ABS_PORT_DATA;
        observation->bus_status = TI_C54X_BUS_OK;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        return;
    }
    /* Second leg: the observed port word written to DATA. */
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(request->address_word == UINT32_C(0x0008));
    TI_C54X_TEST_CHECK(request->write_value_word == PORTR_ABS_PORT_DATA);
    fixture->data_writes++;
    fixture->last_data_address = request->address_word;
    fixture->last_data_value = request->write_value_word;
    observation->bus_status = TI_C54X_BUS_OK;
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

static void
test_absolute_portr_retires(void)
{
    /* PORTR *(0x0008),0x2D reads 0x1234 from port 0x2D
     * and writes it to DATA 0x0008 with no status change. */
    portr_abs_fixture fixture;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    const ti_c54x_bus bus = {
        .context = &fixture,
        .program_read_word = read_program,
        .data_endpoint = {.context = &fixture,
                          .advance = advance_data,
                          .cancel = cancel_data}};
    const ti_c54x_config config = {UINT32_C(0x0100), {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;

    memset(&fixture, 0, sizeof(fixture));
    fixture.base = UINT32_C(0x0100);
    fixture.words[0] = PORTR_ABS_WORD0;
    fixture.words[1] = PORTR_ABS_ADDR;
    fixture.words[2] = PORTR_ABS_PA;
    fixture.word_count = 3U;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0103));
    TI_C54X_TEST_CHECK(fixture.io_reads == 1U);
    TI_C54X_TEST_CHECK(fixture.data_writes == 1U);
    TI_C54X_TEST_CHECK(fixture.last_data_address == UINT32_C(0x0008));
    TI_C54X_TEST_CHECK(fixture.last_data_value == PORTR_ABS_PORT_DATA);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_absolute_portr_retires();
    return 0;
}
