/* Module contract
 * Owns: public-CPU RED proof for absolute ST #lk (P3-S70,
 * TI SPRU172C section ST syntax 3 with absolute
 * addressing): `ST #4,*(0x803)` retires into endpoint RAM
 * with zero MMR traffic, mirroring the loader1 word at
 * 0xF0C.
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

#define ST_ABS_WORD0 UINT16_C(0x76F8)
#define ST_ABS_WORD1 UINT16_C(0x0803)
#define ST_ABS_WORD2 UINT16_C(0x0004)
#define ST_ABS_RAM_BASE UINT32_C(0x0800)
#define ST_ABS_RAM_WORDS UINT32_C(0x0400)

typedef struct st_abs_fixture {
    uint32_t base;
    uint16_t words[3];
    unsigned word_count;
    unsigned program_calls;
    uint16_t ram[0x0400];
    unsigned data_calls;
    unsigned data_writes;
    uint32_t last_write_address;
    uint16_t last_write_value;
} st_abs_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    st_abs_fixture *fixture = (st_abs_fixture *)context;
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
    st_abs_fixture *fixture = (st_abs_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->data_calls++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if (request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
        request->address_word >= ST_ABS_RAM_BASE &&
        (uint32_t)(request->address_word - ST_ABS_RAM_BASE) <
            ST_ABS_RAM_WORDS) {
        uint32_t index =
            (uint32_t)(request->address_word - ST_ABS_RAM_BASE);
        if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE) {
            fixture->ram[index] = request->write_value_word;
            fixture->data_writes++;
            fixture->last_write_address = request->address_word;
            fixture->last_write_value = request->write_value_word;
        }
        observation->bus_status = TI_C54X_BUS_OK;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        return;
    }
    observation->bus_status = TI_C54X_BUS_FAULT;
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
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
test_absolute_store_retires_to_ram(void)
{
    st_abs_fixture fixture;
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
    fixture.words[0] = ST_ABS_WORD0;
    fixture.words[1] = ST_ABS_WORD1;
    fixture.words[2] = ST_ABS_WORD2;
    fixture.word_count = 3U;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.data_writes == 1U);
    TI_C54X_TEST_CHECK(fixture.last_write_address == UINT32_C(0x0803));
    TI_C54X_TEST_CHECK(fixture.last_write_value == UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_absolute_store_retires_to_ram();
    return 0;
}
