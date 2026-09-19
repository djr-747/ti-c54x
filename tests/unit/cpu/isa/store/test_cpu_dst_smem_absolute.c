/* Module contract
 * Owns: public-CPU RED proof for absolute DST src,Smem
 * (P3-S74, TI SPRU172C section DST with absolute
 * addressing): `DST A,*(0x60)` retires the 32-bit
 * accumulator word pair into endpoint DATA space (MSW
 * at the absolute address, LSW above it), mirroring the
 * loader1 word at 0xF71.
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

#define DST_ABS_WORD0 UINT16_C(0x4EF8)
#define DST_ABS_WORD1 UINT16_C(0x0060)
#define DST_ABS_SOURCE_VALUE UINT64_C(0x0012345678)

typedef struct dst_abs_fixture {
    uint32_t base;
    uint16_t words[2];
    unsigned word_count;
    unsigned data_writes;
    uint32_t write_addresses[2];
    uint16_t write_values[2];
} dst_abs_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    dst_abs_fixture *fixture = (dst_abs_fixture *)context;
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
    dst_abs_fixture *fixture = (dst_abs_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture->data_writes < 2U);
    fixture->write_addresses[fixture->data_writes] =
        request->address_word;
    fixture->write_values[fixture->data_writes] =
        request->write_value_word;
    fixture->data_writes++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
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
test_absolute_store_retires_long32(void)
{
    dst_abs_fixture fixture;
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
    fixture.words[0] = DST_ABS_WORD0;
    fixture.words[1] = DST_ABS_WORD1;
    fixture.word_count = 2U;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           DST_ABS_SOURCE_VALUE) == TI_C54X_ACCUMULATOR_OK);
    cpu->arch_state.status.st1 =
        (uint16_t)(cpu->arch_state.status.st1 &
                   (uint16_t)~TI_C54X_ST1_SXM_MASK);
    cpu->arch_state.status.pmst =
        (uint16_t)(cpu->arch_state.status.pmst &
                   (uint16_t)~TI_C54X_PMST_SST_MASK);
    cpu->arch_state.status.st1_known_mask =
        (uint16_t)(cpu->arch_state.status.st1_known_mask |
                   TI_C54X_ST1_SXM_MASK);
    cpu->arch_state.status.pmst_known_mask =
        (uint16_t)(cpu->arch_state.status.pmst_known_mask |
                   TI_C54X_PMST_SST_MASK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0102));
    /* MSW issues first at the absolute address, LSW above. */
    TI_C54X_TEST_CHECK(fixture.data_writes == 2U);
    TI_C54X_TEST_CHECK(fixture.write_addresses[0] == UINT32_C(0x0060));
    TI_C54X_TEST_CHECK(fixture.write_addresses[1] == UINT32_C(0x0061));
    TI_C54X_TEST_CHECK(fixture.write_values[0] == UINT16_C(0x1234));
    TI_C54X_TEST_CHECK(fixture.write_values[1] == UINT16_C(0x5678));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_absolute_store_retires_long32();
    return 0;
}
