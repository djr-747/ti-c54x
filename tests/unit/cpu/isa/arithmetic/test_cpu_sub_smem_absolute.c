/* Module contract
 * Owns: public-CPU RED proof for absolute SUB *(abs),src
 * (P3-S82, TI SPRU172C section SUB syntax 1 with
 * absolute addressing): the loader1 word triple at
 * 0xF27 retires `LD *(LD_ADDR),16,A`, `OR *(OR_ADDR),A`,
 * then `SUB *(SUB_ADDR),A`, subtracting the endpoint
 * DATA word from accumulator A with C written from the
 * bit-32 borrow.
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

#define SUB_ABS_LD_WORD0 UINT16_C(0x44F8)
#define SUB_ABS_LD_ADDR UINT16_C(0x0060)
#define SUB_ABS_LD_DATA UINT16_C(0x1234)
#define SUB_ABS_OR_WORD0 UINT16_C(0x1AF8)
#define SUB_ABS_OR_ADDR UINT16_C(0x0061)
#define SUB_ABS_OR_DATA UINT16_C(0x00FF)
#define SUB_ABS_WORD0 UINT16_C(0x08F8)
#define SUB_ABS_ADDR UINT16_C(0x0062)
#define SUB_ABS_DATA UINT16_C(0x0001)

typedef struct sub_abs_fixture {
    uint32_t base;
    uint16_t words[6];
    unsigned word_count;
    unsigned data_reads;
} sub_abs_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    sub_abs_fixture *fixture = (sub_abs_fixture *)context;
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
    sub_abs_fixture *fixture = (sub_abs_fixture *)context;
    uint16_t data = SUB_ABS_DATA;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(request->address_word == UINT32_C(0x0060) ||
                       request->address_word == UINT32_C(0x0061) ||
                       request->address_word == UINT32_C(0x0062));
    if (request->address_word == UINT32_C(0x0060)) {
        data = SUB_ABS_LD_DATA;
    } else if (request->address_word == UINT32_C(0x0061)) {
        data = SUB_ABS_OR_DATA;
    }
    fixture->data_reads++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    observation->read_value_word = data;
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
test_absolute_sub_retires(void)
{
    /* SXM=0, OVM=0: LD widens 0x1234 into A(31:16),
     * OR merges 0x00FF, then SUB takes 1 back with no
     * borrow: A = 0x00123400FE. */
    sub_abs_fixture fixture;
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
    fixture.words[0] = SUB_ABS_LD_WORD0;
    fixture.words[1] = SUB_ABS_LD_ADDR;
    fixture.words[2] = SUB_ABS_OR_WORD0;
    fixture.words[3] = SUB_ABS_OR_ADDR;
    fixture.words[4] = SUB_ABS_WORD0;
    fixture.words[5] = SUB_ABS_ADDR;
    fixture.word_count = 6U;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1_known_mask &
                        TI_C54X_ST1_SXM_MASK) != UINT16_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(3));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(3));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(3));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0106));
    TI_C54X_TEST_CHECK(fixture.data_reads == 3U);
    TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                       UINT64_C(0x00123400FE));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_absolute_sub_retires();
    return 0;
}
