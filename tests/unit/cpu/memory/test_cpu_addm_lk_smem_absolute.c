/* Module contract
 * Owns: public-CPU RED proof for absolute ADDM #lk,Smem
 * (P3-S76, TI SPRU172C section ADDM with absolute
 * addressing): `ADDM #0x800,*(0x13)` retires the MMR-routed
 * AR3 cell plus immediate back into AR3 with no endpoint
 * traffic, mirroring the loader1 word at 0xF1F.
 * Does not own: production CPU behavior, catalog/decode or pure
 * effects, address planning, shared fixtures, timing,
 * diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../src/cpu/cpu_internal.h"
#include "../../../../src/state/register_layout.h"
#include "../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define ADDM_ABS_WORD0 UINT16_C(0x6BF8)
#define ADDM_ABS_WORD1 UINT16_C(0x0013)
#define ADDM_ABS_WORD2 UINT16_C(0x0800)
#define ADDM_ABS_READ_VALUE UINT16_C(0x0100)
#define ADDM_ABS_EXPECTED_WRITE UINT16_C(0x0900)

typedef struct addm_abs_fixture {
    uint32_t base;
    uint16_t words[3];
    unsigned word_count;
    unsigned data_reads;
    unsigned data_writes;
    uint32_t last_write_address;
    uint16_t last_write_value;
} addm_abs_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    addm_abs_fixture *fixture = (addm_abs_fixture *)context;
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
    addm_abs_fixture *fixture = (addm_abs_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->address_word == UINT32_C(0x0013));
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if (request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ) {
        fixture->data_reads++;
        observation->read_value_word = ADDM_ABS_READ_VALUE;
        observation->bus_status = TI_C54X_BUS_OK;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
        return;
    }
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    fixture->data_writes++;
    fixture->last_write_address = request->address_word;
    fixture->last_write_value = request->write_value_word;
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
test_absolute_add_retires_readback(void)
{
    /* OVM=0/SXM=0: the 0x0100 cell plus 0x0800 writes back
     * 0x0900 with no saturation. */
    addm_abs_fixture fixture;
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
    fixture.words[0] = ADDM_ABS_WORD0;
    fixture.words[1] = ADDM_ABS_WORD1;
    fixture.words[2] = ADDM_ABS_WORD2;
    fixture.word_count = 3U;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 &
                   (uint16_t)~TI_C54X_ST1_SXM_MASK &
                   (uint16_t)~TI_C54X_ST1_OVM_MASK);
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_SXM_MASK | TI_C54X_ST1_OVM_MASK);
    /* DATA 0x13 is MMR-routed AR3: seed it known so the RMW read
     * is served locally; the endpoint must stay silent. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state,
                                                  UINT8_C(3),
                                                  ADDM_ABS_READ_VALUE) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0103));
    TI_C54X_TEST_CHECK(fixture.data_reads == 0U);
    TI_C54X_TEST_CHECK(fixture.data_writes == 0U);
    {
        uint16_t ar3 = UINT16_C(0);
        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                     UINT8_C(3),
                                                     &ar3) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(ar3 == ADDM_ABS_EXPECTED_WRITE);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_absolute_add_retires_readback();
    return 0;
}
