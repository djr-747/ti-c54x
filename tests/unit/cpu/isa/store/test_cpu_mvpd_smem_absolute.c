/* Module contract
 * Owns: public-CPU RED proof for absolute MVPD pmad,Smem
 * (P3-S73, TI SPRU172C section MVPD with absolute
 * addressing): `MVPD 0xFF87,*(0x802)` retires the endpoint
 * PROGRAM word into DATA space, mirroring the loader1
 * word at 0xF12.
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

#define MVPD_ABS_WORD0 UINT16_C(0x7CF8)
#define MVPD_ABS_WORD1 UINT16_C(0x0802)
#define MVPD_ABS_WORD2 UINT16_C(0xFF87)
#define MVPD_ABS_RAM_BASE UINT32_C(0x0800)
#define MVPD_ABS_RAM_WORDS UINT32_C(0x0400)
#define MVPD_ABS_PROGRAM_VALUE UINT16_C(0x0004)

typedef struct mvpd_abs_fixture {
    uint32_t base;
    uint16_t words[3];
    unsigned word_count;
    unsigned program_calls;
    uint16_t ram[0x0400];
    unsigned data_calls;
    unsigned data_writes;
    uint32_t last_data_address;
    uint16_t last_data_value;
} mvpd_abs_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    mvpd_abs_fixture *fixture = (mvpd_abs_fixture *)context;
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
    mvpd_abs_fixture *fixture = (mvpd_abs_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->data_calls++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if (request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_PROGRAM &&
        request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ &&
        request->address_word == MVPD_ABS_WORD2) {
        observation->read_value_word = MVPD_ABS_PROGRAM_VALUE;
        observation->bus_status = TI_C54X_BUS_OK;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        return;
    }
    if (request->space == TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA &&
        request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE &&
        request->address_word >= MVPD_ABS_RAM_BASE &&
        (uint32_t)(request->address_word - MVPD_ABS_RAM_BASE) <
            MVPD_ABS_RAM_WORDS) {
        fixture->data_writes++;
        fixture->last_data_address = request->address_word;
        fixture->last_data_value = request->write_value_word;
        fixture->ram[(uint32_t)(request->address_word -
                                MVPD_ABS_RAM_BASE)] =
            request->write_value_word;
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
test_absolute_move_retires_to_data(void)
{
    mvpd_abs_fixture fixture;
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
    fixture.words[0] = MVPD_ABS_WORD0;
    fixture.words[1] = MVPD_ABS_WORD1;
    fixture.words[2] = MVPD_ABS_WORD2;
    fixture.word_count = 3U;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0103));
    TI_C54X_TEST_CHECK(fixture.data_writes == 1U);
    TI_C54X_TEST_CHECK(fixture.last_data_address ==
                       UINT32_C(0x0802));
    TI_C54X_TEST_CHECK(fixture.last_data_value == MVPD_ABS_PROGRAM_VALUE);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_absolute_move_retires_to_data();
    return 0;
}
