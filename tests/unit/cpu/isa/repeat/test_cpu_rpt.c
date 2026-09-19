/* Module contract
 * Owns: public-CPU RED proofs for RPT activation, including RC
 * load and sequential retirement for `RPT #K` and `RPT #lk`,
 * the eligible NOP delay-loop body, transactional body
 * blocks for UNRESOLVED (STM) and INELIGIBLE bodies
 * (including nested RPT), latched-request persistence
 * across activation and blocks, and reset clearing.
 * Does not own: production CPU behavior, catalog/decode or pure
 * repeat effects, the single-repeat driver, shared fixtures,
 * timing, diagnostics, or test manifests.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/sequencer/interrupt/repeat_pending_request.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/interrupt_flag_register.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/repeat_counter_register.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../../src/state/temporary/t_register.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t NOP_WORD = UINT16_C(0xF495);
static const uint16_t RET_WORD = UINT16_C(0xFC00);

typedef struct rpt_fixture {
    uint16_t words[8];
    unsigned word_count;
    ti_c54x_program_word_address start;
    unsigned program_calls;
    unsigned endpoint_calls;
} rpt_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    rpt_fixture *fixture = (rpt_fixture *)context;
    uint32_t offset;

    /* HOLD re-drives body fetches at a repeated address (P3-S69
     * eligible NOP loop), so serve by address rather than
     * asserting a monotonic call order. */
    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address >= fixture->start);
    offset = address - fixture->start;
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
    rpt_fixture *fixture = (rpt_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(transaction_id != UINT64_C(0));
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->endpoint_calls++;
    *observation = (ti_c54x_bus_word16_observation){
        .transaction_id = transaction_id,
        .bus_status = TI_C54X_BUS_OK,
        .read_value_word = UINT16_C(0),
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
init_cpu(rpt_fixture *fixture, unsigned char *storage,
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

static void check_rpt_k_retires(void)
{
    rpt_fixture fixture = {.words = {UINT16_C(0xEC02), NOP_WORD},
                           .word_count = 2U,
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(read_rc_value(cpu) == UINT16_C(2));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE);
    TI_C54X_TEST_CHECK(cpu->repeat_progress.loaded_rc_operand ==
                       UINT16_C(2));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.body_executions_remaining ==
                       UINT32_C(3));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void check_unresolved_body_blocks(void)
{
    /* P3-S69: NOP is ELIGIBLE now, so the UNRESOLVED example
     * body is STM #0x2468,T (two words, no read inputs). */
    rpt_fixture fixture = {.words = {UINT16_C(0xEC02), UINT16_C(0x770E),
                                     UINT16_C(0x2468)},
                           .word_count = 3U,
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    /* The UNRESOLVED STM body blocks before extension fetch or
     * dispatch: one program read, nothing retired, PC frozen. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(read_rc_value(cpu) == UINT16_C(2));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void check_rpt_k_zero(void)
{
    rpt_fixture fixture = {.words = {UINT16_C(0xEC00), NOP_WORD},
                           .word_count = 2U,
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    /* Operand zero still activates with a single execution. */
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(read_rc_value(cpu) == UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE);
    TI_C54X_TEST_CHECK(cpu->repeat_progress.body_executions_remaining ==
                       UINT32_C(1));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void check_rpt_lk_max(void)
{
    rpt_fixture fixture = {.words = {UINT16_C(0xF070), UINT16_C(0xFFFF),
                                     NOP_WORD},
                           .word_count = 3U,
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    /* The worked #0xffff example: 65536 executions, RC loaded. */
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(read_rc_value(cpu) == UINT16_C(0xFFFF));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.body_executions_remaining ==
                       UINT32_C(65536));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0102));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void check_nested_rpt_blocks(void)
{
    rpt_fixture fixture = {.words = {UINT16_C(0xEC02), UINT16_C(0xEC03)},
                           .word_count = 2U,
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    /* RPT is repeat-control: as a body it rejects INELIGIBLE with
     * the first RC load intact. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(read_rc_value(cpu) == UINT16_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void check_ineligible_body_blocks(void)
{
    rpt_fixture fixture = {.words = {UINT16_C(0xEC01), RET_WORD},
                           .word_count = 2U,
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    /* RET is control-transfer: INELIGIBLE rejects before dispatch,
     * so no stack traffic issues. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(read_rc_value(cpu) == UINT16_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0101));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void check_latched_request_persists(void)
{
    /* P3-S69: STM body keeps the UNRESOLVED block (see
     * check_unresolved_body_blocks). */
    rpt_fixture fixture = {.words = {UINT16_C(0xEC02), UINT16_C(0x770E),
                                     UINT16_C(0x2468)},
                           .word_count = 3U,
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_sequencer_repeat_request_decision decision =
        TI_C54X_SEQUENCER_REPEAT_REQUEST_DECISION_INVALID;
    ti_c54x_run_result result;
    uint16_t ifr = UINT16_C(0);

    /* Latch a maskable request before activation. */
    TI_C54X_TEST_CHECK(ti_c54x_repeat_pending_request_receive(
                           &cpu->arch_state, &cpu->repeat_pending,
                           UINT16_C(0x0004), UINT32_C(0), &decision) ==
                       TI_C54X_REPEAT_PENDING_REQUEST_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    /* Activation retires with the request still latched: nothing
     * taken, IFR intact. */
    TI_C54X_TEST_CHECK(cpu->repeat_pending.phase ==
                       TI_C54X_REPEAT_PENDING_REQUEST_LATCHED);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ifr(&cpu->arch_state,
                                                   &ifr) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK((uint16_t)(ifr & UINT16_C(0x0004)) ==
                       UINT16_C(0x0004));
    /* The body block keeps deferral: latch and IFR persist. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(cpu->repeat_pending.phase ==
                       TI_C54X_REPEAT_PENDING_REQUEST_LATCHED);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_ifr(&cpu->arch_state,
                                                   &ifr) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK((uint16_t)(ifr & UINT16_C(0x0004)) ==
                       UINT16_C(0x0004));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void check_reset_clears_repeat(void)
{
    rpt_fixture fixture = {.words = {UINT16_C(0xEC02), NOP_WORD},
                           .word_count = 2U,
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(1));

    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(cpu->repeat_progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_reset(cpu) == TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(cpu->repeat_progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_IDLE);
    TI_C54X_TEST_CHECK(cpu->repeat_progress.body_executions_remaining ==
                       UINT32_C(0));
    TI_C54X_TEST_CHECK(cpu->repeat_pending.phase ==
                       TI_C54X_REPEAT_PENDING_REQUEST_EMPTY);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0100));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void check_eligible_nop_body_retires(void)
{
    rpt_fixture fixture = {.words = {UINT16_C(0xEC02), NOP_WORD},
                           .word_count = 2U,
                           .start = 0U,
                           .program_calls = 0U,
                           .endpoint_calls = 0U};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu =
        init_cpu(&fixture, storage, UINT32_C(0x0100));
    ti_c54x_run_result result = ti_c54x_cpu_run(cpu, UINT64_C(4));

    /* P3-S69: NOP is absent from Table 2-28, so the RPT #2
     * delay-loop idiom retires all four instructions. */
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(4));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(4));
    TI_C54X_TEST_CHECK(fixture.program_calls == 4U);
    TI_C54X_TEST_CHECK(fixture.endpoint_calls == 0U);
    TI_C54X_TEST_CHECK(read_rc_value(cpu) == UINT16_C(0));
    TI_C54X_TEST_CHECK(cpu->repeat_progress.phase ==
                       TI_C54X_REPEAT_PROGRESS_PHASE_COMPLETE);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0102));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    check_rpt_k_retires();
    check_unresolved_body_blocks();
    check_eligible_nop_body_retires();
    check_rpt_k_zero();
    check_rpt_lk_max();
    check_nested_rpt_blocks();
    check_ineligible_body_blocks();
    check_latched_request_persists();
    check_reset_clears_repeat();
    return 0;
}
