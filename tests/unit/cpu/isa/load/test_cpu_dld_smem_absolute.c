/* Module contract
 * Owns: public-CPU RED proof for absolute DLD *(abs),dst
 * (P3-S78, TI SPRU172C section DLD with absolute
 * addressing): `DLD *(0x60),A` retires the endpoint DATA
 * long-word pair into accumulator A with SXM widening
 * (MSW at the absolute address, LSW above it),
 * mirroring the loader1 word at 0x8024.
 * Does not own: production CPU behavior, catalog/decode or pure
 * effects, address planning, shared fixtures, timing,
 * diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define DLD_ABS_WORD0 UINT16_C(0x56F8)
#define DLD_ABS_WORD1 UINT16_C(0x0060)
#define DLD_ABS_MSW UINT16_C(0x8001)
#define DLD_ABS_LSW UINT16_C(0x0002)

typedef struct dld_abs_fixture {
    uint32_t base;
    uint16_t words[2];
    unsigned word_count;
    unsigned data_reads;
    uint32_t read_addresses[2];
} dld_abs_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    dld_abs_fixture *fixture = (dld_abs_fixture *)context;
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
    dld_abs_fixture *fixture = (dld_abs_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture->data_reads < 2U);
    fixture->read_addresses[fixture->data_reads] =
        request->address_word;
    fixture->data_reads++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    observation->read_value_word =
        (fixture->data_reads == 1U) ? DLD_ABS_MSW : DLD_ABS_LSW;
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
test_absolute_load_retires_long32(void)
{
    /* SXM=1 sign-extends the 0x80010002 long word into A. */
    dld_abs_fixture fixture;
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
    fixture.words[0] = DLD_ABS_WORD0;
    fixture.words[1] = DLD_ABS_WORD1;
    fixture.word_count = 2U;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 | TI_C54X_ST1_SXM_MASK);
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_SXM_MASK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0102));
    /* MSW reads first at the absolute address, LSW above. */
    TI_C54X_TEST_CHECK(fixture.data_reads == 2U);
    TI_C54X_TEST_CHECK(fixture.read_addresses[0] == UINT32_C(0x0060));
    TI_C54X_TEST_CHECK(fixture.read_addresses[1] == UINT32_C(0x0061));
    TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                       UINT64_C(0xFF80010002));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_absolute_load_retires_long32();
    return 0;
}
