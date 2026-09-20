/* Module contract
 * Owns: public-CPU RED proofs for `RPT Smem` activation,
 * including RC load from direct and indirect Smem words,
 * the DATA READ address, body repetition Smem+1 times,
 * and the nested-RPT body block. Does not own:
 * production CPU behavior, catalog/decode or pure repeat
 * effects, the single-repeat driver, shared fixtures,
 * timing, diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/repeat_counter_register.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t NOP_WORD = UINT16_C(0xF495);
static const uint16_t RPT_K_WORD = UINT16_C(0xEC02);

typedef struct rpt_smem_fixture {
    uint16_t words[8];
    unsigned word_count;
    uint16_t read_value_word;
    unsigned program_calls;
    unsigned endpoint_calls;
    ti_c54x_bus_word16_request last_request;
} rpt_smem_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    rpt_smem_fixture *fixture = (rpt_smem_fixture *)context;
    uint32_t offset;

    /* HOLD re-drives body fetches at a repeated address, so
     * serve by address rather than asserting call order. */
    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address < fixture->word_count);
    offset = address;
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
    rpt_smem_fixture *fixture = (rpt_smem_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->last_request = *request;
    fixture->endpoint_calls++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    observation->bus_status = TI_C54X_BUS_OK;
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
    observation->read_value_word =
        request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
            ? fixture->read_value_word
            : UINT16_C(0);
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

static ti_c54x_cpu *
init_cpu(rpt_smem_fixture *fixture, unsigned char *storage)
{
    const ti_c54x_bus bus = {
        .context = fixture,
        .program_read_word = read_program,
        .data_endpoint = {.context = fixture,
                          .advance = advance_data,
                          .cancel = cancel_data}};
    const ti_c54x_config config = {UINT32_C(0), {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

static uint16_t
read_rc_value(const ti_c54x_cpu *cpu)
{
    uint16_t rc = UINT16_C(0xBEEF);

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_rc(&cpu->arch_state, &rc) ==
                       TI_C54X_ARCH_STATE_OK);
    return rc;
}

static void
test_direct_repeats_body(void)
{
    /* DP 0 with dma 5 reads DATA 0x0005; the scripted Smem
     * word 2 repeats the NOP body 3 times. */
    rpt_smem_fixture fixture = {.words = {UINT16_C(0x4705), NOP_WORD,
                                          NOP_WORD},
                                .word_count = 3U,
                                .read_value_word = UINT16_C(0x0002)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&cpu->arch_state,
                                                  UINT16_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(fixture.last_request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture.last_request.address_word ==
                       UINT16_C(0x0005));
    TI_C54X_TEST_CHECK(read_rc_value(cpu) == UINT16_C(2));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE);
    TI_C54X_TEST_CHECK(cpu->repeat_progress.loaded_rc_operand ==
                       UINT16_C(2));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.body_executions_remaining ==
                       UINT32_C(3));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(1));

    /* The body runs exactly 3 times, then execution advances
     * past it: 3 body retirements plus the next NOP. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(4));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(4));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(3));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_indirect_loads_rc(void)
{
    /* I=1 MOD0 AR2 reads DATA AR2 with no displacement. */
    rpt_smem_fixture fixture = {.words = {UINT16_C(0x4782), NOP_WORD},
                                .word_count = 2U,
                                .read_value_word = UINT16_C(0x0000)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state,
                                                    UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state,
                                                   UINT8_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_ar(&cpu->arch_state,
                                                  UINT8_C(2),
                                                  UINT16_C(0x2110)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.last_request.address_word ==
                       UINT16_C(0x2110));
    TI_C54X_TEST_CHECK(read_rc_value(cpu) == UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE);
    TI_C54X_TEST_CHECK(cpu->repeat_progress.body_executions_remaining ==
                       UINT32_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_nested_rpt_body_blocks(void)
{
    /* An INELIGIBLE RPT body under an active RPT Smem blocks
     * without retiring the body. */
    rpt_smem_fixture fixture = {.words = {UINT16_C(0x4705), RPT_K_WORD,
                                          NOP_WORD},
                                .word_count = 3U,
                                .read_value_word = UINT16_C(0x0001)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage);
    ti_c54x_run_result result;

    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_dp(&cpu->arch_state,
                                                  UINT16_C(0)) ==
                       TI_C54X_ARCH_STATE_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_direct_repeats_body();
    test_indirect_loads_rc();
    test_nested_rpt_body_blocks();
    return 0;
}
