/* Module contract
 * Owns: public-CPU RED proof for MAR *AR2- (P3-S77, TI
 * SPRU172C section MAR, modify auxiliary register):
 * `MAR *AR2-` retires the postdecremented AR2 cell
 * locally with no endpoint traffic, mirroring the
 * loader word at 0x8023, and rejects transactionally
 * on unknown AR2.
 * Does not own: production CPU behavior, catalog/decode or pure
 * effects, address planning, shared fixtures, timing,
 * diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/arch_state.h"
#include "../../../../support/test_harness.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MAR_SMEM_WORD UINT16_C(0x6D8A)
#define MAR_AR2_SEED UINT16_C(0x1234)
#define MAR_AR2_EXPECTED_WRITE UINT16_C(0x1233)

typedef struct mar_fixture {
    uint32_t base;
    uint16_t words[1];
    unsigned word_count;
    unsigned endpoint_calls;
} mar_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    mar_fixture *fixture = (mar_fixture *)context;
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
    mar_fixture *fixture = (mar_fixture *)context;

    (void)phase;
    (void)transaction_id;
    (void)request;
    (void)observation;
    /* MAR retires with no endpoint traffic: any call fails. */
    fixture->endpoint_calls++;
    TI_C54X_TEST_CHECK(fixture->endpoint_calls == 0U);
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
test_mar_postdec_retires(void)
{
    /* AR2 0x1234 minus one writes back 0x1233 with no
     * saturation model and no bus traffic. */
    mar_fixture fixture;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_bus bus = {0};
    const ti_c54x_config config = {UINT32_C(0x0100), {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;
    uint16_t ar2 = UINT16_C(0);

    memset(&fixture, 0, sizeof(fixture));
    fixture.base = UINT32_C(0x0100);
    fixture.words[0] = MAR_SMEM_WORD;
    fixture.word_count = 1U;
    bus.context = &fixture;
    bus.program_read_word = read_program;
    bus.data_endpoint.context = &fixture;
    bus.data_endpoint.advance = advance_data;
    bus.data_endpoint.cancel = cancel_data;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(),
                                        &config, &bus,
                                        &cpu) == TI_C54X_ERROR_OK);
    /* Seed AR2 known so the postdecrement input collects. */
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state,
                                                  UINT8_C(2),
                                                  MAR_AR2_SEED) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ar(&cpu->arch_state,
                                                 UINT8_C(2),
                                                 &ar2) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ar2 == MAR_AR2_EXPECTED_WRITE);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_mar_unknown_ar2_rejects(void)
{
    /* Unknown AR2 fails closed before any retirement. */
    mar_fixture fixture;
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_bus bus = {0};
    const ti_c54x_config config = {UINT32_C(0x0100), {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;
    ti_c54x_run_result result;

    memset(&fixture, 0, sizeof(fixture));
    fixture.base = UINT32_C(0x0100);
    fixture.words[0] = MAR_SMEM_WORD;
    fixture.word_count = 1U;
    bus.context = &fixture;
    bus.program_read_word = read_program;
    bus.data_endpoint.context = &fixture;
    bus.data_endpoint.advance = advance_data;
    bus.data_endpoint.cancel = cancel_data;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(),
                                        &config, &bus,
                                        &cpu) == TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_mar_postdec_retires();
    test_mar_unknown_ar2_rejects();
    return 0;
}
