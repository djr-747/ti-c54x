/* Module contract
 * Owns: public-CPU RED proofs for the P3-S94 NMI take: arrival
 * rejections and lossless pending, the transactional take (parked
 * PC pushed to SP-1, INTM set, PC to the device-owned vector, no
 * RTN move), WAIT/FAULT/unknown-SP transactionality, repeat and
 * delay deferral, IDLE-park wake, and the ISR RETE round trip.
 * Does not own: production CPU behavior, catalog/decode or pure
 * take effects, address planning, shared fixtures, maskable
 * arbitration, timing, diagnostics, or test registration outside
 * the status leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../../src/sequencer/delayed_branch/delayed_branch.h"
#include "../../../../../src/sequencer/repeat/repeat_block.h"
#include "../../../../../src/sequencer/repeat/repeat_progress.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t NOP_WORD0 = UINT16_C(0xF495);
static const uint16_t IDLE_WORD0 = UINT16_C(0xF4E1);
static const uint16_t RETE_WORD0 = UINT16_C(0xF4EB);

typedef struct nmi_take_fixture {
    uint32_t program_addresses[8];
    uint16_t program_words[8];
    unsigned program_slots;
    unsigned program_calls;
    bool data_wait_then_ready;
    bool data_fault;
    bool data_waited;
    uint16_t data_tos_value;
    ti_c54x_bus_word16_request data_request;
    unsigned data_calls;
} nmi_take_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    nmi_take_fixture *fixture = (nmi_take_fixture *)context;
    unsigned i;

    TI_C54X_TEST_CHECK(value != NULL);
    for (i = 0U; i < fixture->program_slots; i++) {
        if (fixture->program_addresses[i] == address) {
            *value = fixture->program_words[i];
            fixture->program_calls++;
            return TI_C54X_BUS_OK;
        }
    }
    return TI_C54X_BUS_FAULT;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    nmi_take_fixture *fixture = (nmi_take_fixture *)context;

    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->data_request = *request;
    fixture->data_calls++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if (fixture->data_fault) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
    if (fixture->data_wait_then_ready &&
        phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
        !fixture->data_waited) {
        fixture->data_waited = true;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        return;
    }
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
    /* WRITE observations carry no read data (bus rule: nonzero
     * read values on WRITE READY reject); only READs return the
     * scripted TOS word. */
    observation->read_value_word =
        request->access == TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ
            ? fixture->data_tos_value
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
init_cpu(nmi_take_fixture *fixture, unsigned char *storage,
         uint32_t reset_pc)
{
    const ti_c54x_bus bus = {
        .context = fixture,
        .program_read_word = read_program,
        .data_endpoint = {.context = fixture,
                          .advance = advance_data,
                          .cancel = cancel_data}};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* All-known status with INTM clear (ST1 bit 11 = 0) so the take's
 * INTM set is observable; SP seeded known. */
static void
seed_cpu(ti_c54x_cpu *cpu, uint16_t sp_value)
{
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x525A);
    cpu->arch_state.status.pmst = UINT16_C(0x3C3C);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
}

static void
test_arrival_rejections_are_lossless(void)
{
    nmi_take_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_state state = {0};

    seed_cpu(cpu, UINT16_C(0x0200));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(NULL, UINT16_C(0x3000)) ==
                       TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu, UINT16_C(0x3000)) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.nmi_pending);
    TI_C54X_TEST_CHECK(state.nmi_vector_word == UINT16_C(0x3000));
    /* A second edge while one is pending rejects without overwrite. */
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu, UINT16_C(0x3100)) ==
                       TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.nmi_pending);
    TI_C54X_TEST_CHECK(state.nmi_vector_word == UINT16_C(0x3000));
    TI_C54X_TEST_CHECK(fixture.program_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_take_pushes_parked_pc_and_sets_intm(void)
{
    nmi_take_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;
    ti_c54x_state state = {0};

    seed_cpu(cpu, UINT16_C(0x0200));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu,
                                                  UINT16_C(0x3000)) ==
                           TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
        TI_C54X_TEST_CHECK(!result.opcode_valid);
        /* No program fetch: the take preempts decode. */
        TI_C54X_TEST_CHECK(fixture.program_calls == 0U);
        /* One TOS WRITE carries the parked PC at old SP - 1. */
        TI_C54X_TEST_CHECK(fixture.data_calls == 1U);
        TI_C54X_TEST_CHECK(fixture.data_request.space ==
                           TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
        TI_C54X_TEST_CHECK(fixture.data_request.access ==
                           TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
        TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                           UINT16_C(0x01FF));
        TI_C54X_TEST_CHECK(fixture.data_request.write_value_word ==
                           UINT16_C(0x1000));
        TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x3000));
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                           UINT32_C(0x3000));
        TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x01FF));
        TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                            UINT16_C(0x0800)) != UINT16_C(0));

        /* Only SP, PC, and the INTM bit move; RTN stays put. */
        {
            ti_c54x_arch_state expect_arch = before_arch;

            expect_arch.pc_word = UINT32_C(0x3000);
            expect_arch.sp = UINT16_C(0x01FF);
            expect_arch.status.st1 = (uint16_t)(before_arch.status.st1 |
                                                UINT16_C(0x0800));
            TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &expect_arch,
                                      sizeof(expect_arch)) == 0);
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == UINT32_C(0x3000));
        TI_C54X_TEST_CHECK(!state.nmi_pending);
        TI_C54X_TEST_CHECK(state.instructions_retired_total ==
                           UINT64_C(1));
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_wait_spans_runs_with_stable_vector(void)
{
    nmi_take_fixture fixture = {.data_wait_then_ready = true};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x0200));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu, UINT16_C(0x3000)) ==
                       TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(fixture.data_calls == 1U);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x3000));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x01FF));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                        UINT16_C(0x0800)) != UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_fault_preserves_edge_and_state(void)
{
    nmi_take_fixture fixture = {.data_fault = true};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;
    ti_c54x_state state = {0};

    seed_cpu(cpu, UINT16_C(0x0200));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu,
                                                  UINT16_C(0x3000)) ==
                           TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.nmi_pending);
        TI_C54X_TEST_CHECK(state.nmi_vector_word == UINT16_C(0x3000));
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_sp_rejects_without_traffic(void)
{
    nmi_take_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;
    ti_c54x_state state = {0};

    /* Fresh reset leaves SP unknown: ST1 seeded all-known only. */
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x525A);
    cpu->arch_state.status.pmst = UINT16_C(0x3C3C);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu,
                                                  UINT16_C(0x3000)) ==
                           TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(fixture.program_calls == 0U);
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.nmi_pending);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_out_of_range_pc_rejects(void)
{
    nmi_take_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x0200));
    cpu->arch_state.pc_word = UINT32_C(0x10000);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu, UINT16_C(0x3000)) ==
                       TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(fixture.program_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_single_repeat_defers(void)
{
    /* Single-ACTIVE defers the take; the fetched IDLE is then
     * refused by the repeat predecode guard (IDLE is
     * INELIGIBLE): the fetch proves arbitration deferred
     * instead of taking, and the edge survives unretired. */
    nmi_take_fixture fixture = {.program_slots = 1U,
                                .program_addresses = {UINT32_C(0x1000)},
                                .program_words = {IDLE_WORD0}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;
    ti_c54x_state state = {0};

    seed_cpu(cpu, UINT16_C(0x0200));
    cpu->repeat_progress.phase = TI_C54X_REPEAT_PROGRESS_PHASE_ACTIVE;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu, UINT16_C(0x3000)) ==
                       TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.nmi_pending);
    TI_C54X_TEST_CHECK(state.nmi_vector_word == UINT16_C(0x3000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_block_repeat_defers(void)
{
    /* Block-ACTIVE defers the take the same way; the forced block
     * state never reaches the retirement tail because IDLE stops
     * first. */
    nmi_take_fixture fixture = {.program_slots = 1U,
                                .program_addresses = {UINT32_C(0x1000)},
                                .program_words = {IDLE_WORD0}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;
    ti_c54x_state state = {0};

    seed_cpu(cpu, UINT16_C(0x0200));
    cpu->repeat_block_progress.phase = TI_C54X_REPEAT_BLOCK_PHASE_ACTIVE;
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu, UINT16_C(0x3000)) ==
                       TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_IDLE);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.nmi_pending);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_delay_countdown_defers(void)
{
    /* An armed delayed-branch countdown defers the take while the
     * ordinary program still retires underneath it. */
    nmi_take_fixture fixture = {.program_slots = 1U,
                                .program_addresses = {UINT32_C(0x1000)},
                                .program_words = {NOP_WORD0}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;
    ti_c54x_state state = {0};

    seed_cpu(cpu, UINT16_C(0x0200));
    TI_C54X_TEST_CHECK(ti_c54x_sequencer_delayed_branch_arm(
                           &cpu->delayed_branch,
                           UINT16_C(0x2000)) ==
                       TI_C54X_SEQUENCER_DELAYED_BRANCH_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu, UINT16_C(0x3000)) ==
                       TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.nmi_pending);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_idle_park_wakes_into_take(void)
{
    nmi_take_fixture fixture = {.program_slots = 1U,
                                .program_addresses = {UINT32_C(0x0F6A)},
                                .program_words = {IDLE_WORD0}};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x0F6A));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x0200));
    result = ti_c54x_cpu_run(cpu, UINT64_C(10));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_IDLE);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x0F6A));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu, UINT16_C(0x3000)) ==
                       TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* The parked IDLE address is what the ISR returns to. */
    TI_C54X_TEST_CHECK(fixture.data_request.write_value_word ==
                       UINT16_C(0x0F6A));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x3000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_rete_round_trip_resumes_parked_program(void)
{
    nmi_take_fixture fixture = {.program_slots = 1U,
                                .program_addresses = {UINT32_C(0x3000)},
                                .program_words = {RETE_WORD0},
                                .data_tos_value = UINT16_C(0x1000)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT16_C(0x0200));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu, UINT16_C(0x3000)) ==
                       TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x3000));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x01FF));
    /* The ISR returns through RETE: TOS pops, INTM clears,
     * the parked PC resumes. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0200));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                        UINT16_C(0x0800)) == UINT16_C(0));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    {
        ti_c54x_state state = {0};

        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.instructions_retired_total ==
                           UINT64_C(2));
        TI_C54X_TEST_CHECK(!state.nmi_pending);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_arrival_rejections_are_lossless();
    test_take_pushes_parked_pc_and_sets_intm();
    test_wait_spans_runs_with_stable_vector();
    test_fault_preserves_edge_and_state();
    test_unknown_sp_rejects_without_traffic();
    test_out_of_range_pc_rejects();
    test_single_repeat_defers();
    test_block_repeat_defers();
    test_delay_countdown_defers();
    test_idle_park_wakes_into_take();
    test_rete_round_trip_resumes_parked_program();
    return 0;
}
