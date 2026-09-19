/* Module contract
 * Owns: public-CPU RED proof for absolute LD *(abs),16,dst
 * (P3-S80, TI SPRU172C section LD with absolute
 * addressing and shift 16): `LD *(0x60),16,A` retires
 * the endpoint DATA word into accumulator A(31:16)
 * with SXM widening plus shift (A(15:0) clear),
 * mirroring the loader1 word at 0xF27.
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

#define LD_ABS_SH16_WORD0 UINT16_C(0x44F8)
#define LD_ABS_SH16_WORD1 UINT16_C(0x0060)
#define LD_ABS_SH16_DATA UINT16_C(0x1234)

typedef struct ld_abs_sh16_fixture {
    uint32_t base;
    uint16_t words[2];
    unsigned word_count;
    unsigned data_reads;
} ld_abs_sh16_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    ld_abs_sh16_fixture *fixture = (ld_abs_sh16_fixture *)context;
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
    ld_abs_sh16_fixture *fixture = (ld_abs_sh16_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(request->address_word == UINT32_C(0x0060));
    fixture->data_reads++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    observation->read_value_word = LD_ABS_SH16_DATA;
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
test_absolute_shift16_retires(void)
{
    /* SXM=0 widens 0x1234 then shifts it into A(31:16). */
    ld_abs_sh16_fixture fixture;
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
    fixture.words[0] = LD_ABS_SH16_WORD0;
    fixture.words[1] = LD_ABS_SH16_WORD1;
    fixture.word_count = 2U;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1_known_mask &
                        TI_C54X_ST1_SXM_MASK) != UINT16_C(0));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0102));
    TI_C54X_TEST_CHECK(fixture.data_reads == 1U);
    TI_C54X_TEST_CHECK(cpu->accumulators.a_raw40 ==
                       UINT64_C(0x0012340000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_absolute_shift16_retires();
    return 0;
}
