/* Module contract
 * Owns: public-CPU RED proofs for single-repeat replay of an ELIGIBLE
 * MAC body, including RC countdown, per-iteration accumulation,
 * HOLD re-drive, terminal ADVANCE, and post-completion state.
 * Does not own: catalog/decode or pure repeat/multiply effects,
 * address-planner internals, endpoint reducer behavior, production
 * code, timing, or diagnostics.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/sequencer/interrupt/repeat_pending_request.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/repeat_counter_register.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../../src/state/temporary/t_register.h"
#include "../../../../support/test_harness.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const uint16_t RPT_K2_WORD = UINT16_C(0xEC02);
static const uint16_t MAC_DIRECT_WORD = UINT16_C(0x2820);
static const uint16_t NOP_WORD = UINT16_C(0xF495);

typedef struct replay_fixture {
    uint16_t words[8];
    unsigned word_count;
    ti_c54x_program_word_address start;
    unsigned program_calls;
    unsigned endpoint_calls;
    uint16_t data_read_value;
} replay_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    replay_fixture *fixture = (replay_fixture *)context;

    /* HOLD re-drives the body fetch at a repeated address, so serve
     * by address rather than asserting a monotonic call order. */
    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address >= fixture->start);
    TI_C54X_TEST_CHECK(address - fixture->start < fixture->word_count);
    *value = fixture->words[address - fixture->start];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    replay_fixture *fixture = (replay_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->endpoint_calls++;
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .bus_status = TI_C54X_BUS_OK,
        .read_value_word = fixture->data_read_value,
        .kind = TI_C54X_BUS_WORD16_OBSERVATION_READY};
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
init_cpu(replay_fixture *fixture, unsigned char *storage,
         ti_c54x_program_word_address start)
{
    const ti_c54x_bus_word16_endpoint endpoint = {
        .context = fixture, .advance = advance_data, .cancel = cancel_data};
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program,
                             .data_endpoint = endpoint};
    const ti_c54x_config config = {start, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    fixture->start = start;
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
test_rpt_mac_replays_three_bodies(void)
{
    replay_fixture fixture = {
        .words = {RPT_K2_WORD, MAC_DIRECT_WORD, NOP_WORD},
        .word_count = 3U,
        .start = 0U,
        .program_calls = 0U,
        .endpoint_calls = 0U,
        .data_read_value = UINT16_C(0x0002)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;
    ti_c54x_state state;

    cpu->t_register.raw_value = UINT16_C(0x0400);
    cpu->t_register.known = true;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0)) == TI_C54X_ACCUMULATOR_OK);

    /* RPT #2 plus three MAC bodies fit a four-instruction budget. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(4));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(4));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(4));
    /* One RPT fetch plus three body fetches; the NOP never executes. */
    TI_C54X_TEST_CHECK(fixture.program_calls == 4U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 3U);
    TI_C54X_TEST_CHECK(read_rc_value(cpu) == UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_COMPLETE);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0102));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    /* Three iterations of 0x0400 * 0x0002 accumulate to 0x1800. */
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x1800));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_single_body_still_advances(void)
{
    replay_fixture fixture = {
        .words = {UINT16_C(0xEC00), MAC_DIRECT_WORD, NOP_WORD},
        .word_count = 3U,
        .start = 0U,
        .program_calls = 0U,
        .endpoint_calls = 0U,
        .data_read_value = UINT16_C(0x0002)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result;
    ti_c54x_state state;

    cpu->t_register.raw_value = UINT16_C(0x0400);
    cpu->t_register.known = true;
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0)) == TI_C54X_ACCUMULATOR_OK);

    /* RPT #0 executes its single body once, then advances. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(3));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(3));
    TI_C54X_TEST_CHECK(fixture.program_calls == 3U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 1U);
    TI_C54X_TEST_CHECK(read_rc_value(cpu) == UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0103));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.accumulator_a_raw40 == UINT64_C(0x0800));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_rpt_mac_replays_three_bodies();
    test_single_body_still_advances();
    return 0;
}
