/* Module contract
 * Owns: public-CPU RED proofs for the P3-S95 maskable take:
 * vector-API rejections and last-wins re-routing, the
 * transactional single-source take (parked PC pushed, exactly
 * its IFR bit acknowledged, INTM set, PC to the routed entry,
 * no RTN move), INTM/IMR/vector/priority deferrals with edges
 * preserved, NMI preemption with WAIT-span survival, and the
 * ISR RETE round trip without re-take.
 * Does not own: production CPU behavior, catalog/decode or pure
 * take effects, address planning, shared fixtures, the NMI
 * path, multi-source priority, timing, diagnostics, or test
 * registration outside the status leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/mmr_file.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../../src/sequencer/delayed_branch/delayed_branch.h"
#include "../../../../../src/sequencer/repeat/repeat_block.h"
#include "../../../../../src/sequencer/repeat/repeat_progress.h"
#include "../../../../support/test_harness.h"
#include "../../../../support/test_arch_state_compare.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t NOP_WORD0 = UINT16_C(0xF495);
static const uint16_t RETE_WORD0 = UINT16_C(0xF4EB);
/* Loader2 doorbell flavor: HPINT mask with its live slot. */
static const uint16_t HPINT_MASK = UINT16_C(0x0200);
static const uint16_t HPINT_VECTOR = UINT16_C(0xFFE4);

typedef struct maskable_take_fixture {
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
} maskable_take_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    maskable_take_fixture *fixture = (maskable_take_fixture *)context;
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
    maskable_take_fixture *fixture = (maskable_take_fixture *)context;

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
    /* WRITE observations carry no read data (bus rule); only
     * READs return the scripted TOS word. */
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
init_cpu(maskable_take_fixture *fixture, unsigned char *storage,
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

/* All-known status with INTM clear (ST1 bit 11 = 0) so the
 * take's INTM set is observable; SP seeded known; IMR seeded
 * known with exactly the HPINT bit enabled. */
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
    TI_C54X_TEST_CHECK(ti_c54x_mmr_file_write_imr(&cpu->arch_state.mmr_file,
                                                 HPINT_MASK) ==
                       TI_C54X_MMR_FILE_OK);
}

static void
test_vector_arrival_rejections_reroute_losslessly(void)
{
    maskable_take_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_state state = {0};

    seed_cpu(cpu, UINT16_C(0x0200));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                           NULL, HPINT_MASK, HPINT_VECTOR) ==
                       TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                           cpu, UINT16_C(0), HPINT_VECTOR) ==
                       TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                           cpu, UINT16_C(0x0201), HPINT_VECTOR) ==
                       TI_C54X_ERROR_INVALID_ARGUMENT);
    TI_C54X_TEST_CHECK(fixture.program_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);

    /* First route latches bit plus entry. */
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                           cpu, HPINT_MASK, HPINT_VECTOR) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.ifr_value == HPINT_MASK);

    /* Re-routing a pending source overwrites the entry and
     * preserves the edge (last wins, lossless). */
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                           cpu, HPINT_MASK, UINT16_C(0x3100)) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.ifr_value == HPINT_MASK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_take_acknowledges_routes_and_sets_intm(void)
{
    maskable_take_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;
    ti_c54x_state state = {0};

    seed_cpu(cpu, UINT16_C(0x0200));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                               cpu, HPINT_MASK, HPINT_VECTOR) ==
                           TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
        TI_C54X_TEST_CHECK(!result.opcode_valid);
        /* No program fetch: the take preempts decode. */
        TI_C54X_TEST_CHECK(fixture.program_calls == 0U);
        /* One TOS WRITE carries the parked PC at old SP - 1. */
        TI_C54X_TEST_CHECK(fixture.data_calls == 1U);
        TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                           UINT16_C(0x01FF));
        TI_C54X_TEST_CHECK(fixture.data_request.write_value_word ==
                           UINT16_C(0x1000));
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                           UINT32_C(0xFFE4));
        TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x01FF));
        TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                            UINT16_C(0x0800)) != UINT16_C(0));
        /* The IFR edge acknowledges: exactly its bit clears. */
        TI_C54X_TEST_CHECK(cpu->arch_state.ifr == UINT16_C(0x0000));

        /* Only SP, PC, INTM, and the IFR bit move; RTN stays. */
        {
            ti_c54x_arch_state expect_arch = before_arch;

            expect_arch.pc_word = UINT32_C(0xFFE4);
            expect_arch.sp = UINT16_C(0x01FF);
            expect_arch.status.st1 = (uint16_t)(before_arch.status.st1 |
                                                UINT16_C(0x0800));
            expect_arch.ifr = UINT16_C(0x0000);
            TI_C54X_TEST_CHECK(ti_c54x_test_arch_state_equal(&cpu->arch_state, &expect_arch));
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.ifr_value == UINT16_C(0x0000));
        TI_C54X_TEST_CHECK(state.instructions_retired_total ==
                           UINT64_C(1));
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_masked_or_unknown_blocks_without_traffic(void)
{
    /* INTM set masks the take; the program proceeds instead. */
    {
        maskable_take_fixture fixture = {.program_slots = 1U,
                                         .program_addresses = {
                                             UINT32_C(0x1000)},
                                         .program_words = {NOP_WORD0}};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage,
                                    UINT32_C(0x1000));
        ti_c54x_run_result result;
        ti_c54x_state state = {0};

        seed_cpu(cpu, UINT16_C(0x0200));
        cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                               cpu, HPINT_MASK, HPINT_VECTOR) ==
                           TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                           UINT32_C(0x1001));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.ifr_value == HPINT_MASK);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                           TI_C54X_ERROR_OK);
    }
    /* Unknown IMR cannot prove enabled: the program runs
     * past the edge with no take traffic. */
    {
        maskable_take_fixture fixture = {.program_slots = 1U,
                                         .program_addresses = {
                                             UINT32_C(0x1000)},
                                         .program_words = {NOP_WORD0}};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage,
                                    UINT32_C(0x1000));
        ti_c54x_run_result result;
        ti_c54x_state state = {0};

        seed_cpu(cpu, UINT16_C(0x0200));
        cpu->arch_state.mmr_file = (ti_c54x_mmr_file_state){0};
        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                               cpu, HPINT_MASK, HPINT_VECTOR) ==
                           TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                           UINT32_C(0x1001));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.ifr_value == HPINT_MASK);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                           TI_C54X_ERROR_OK);
    }
    /* Known IMR without the bit masks it out. */
    {
        maskable_take_fixture fixture = {.program_slots = 1U,
                                         .program_addresses = {
                                             UINT32_C(0x1000)},
                                         .program_words = {NOP_WORD0}};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage,
                                    UINT32_C(0x1000));
        ti_c54x_run_result result;

        seed_cpu(cpu, UINT16_C(0x0200));
        TI_C54X_TEST_CHECK(ti_c54x_mmr_file_write_imr(
                               &cpu->arch_state.mmr_file,
                               UINT16_C(0x0001)) ==
                           TI_C54X_MMR_FILE_OK);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                               cpu, HPINT_MASK, HPINT_VECTOR) ==
                           TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                           UINT32_C(0x1001));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                           TI_C54X_ERROR_OK);
    }
}

static void
test_vectorless_and_priority_gaps_defer(void)
{
    /* S93 vectorless latch: pending and enabled but with no
     * routed entry, so no take. */
    {
        maskable_take_fixture fixture = {.program_slots = 1U,
                                         .program_addresses = {
                                             UINT32_C(0x1000)},
                                         .program_words = {NOP_WORD0}};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage,
                                    UINT32_C(0x1000));
        ti_c54x_run_result result;
        ti_c54x_state state = {0};

        seed_cpu(cpu, UINT16_C(0x0200));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt(
                               cpu, HPINT_MASK) == TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.ifr_value == HPINT_MASK);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                           TI_C54X_ERROR_OK);
    }
    /* Two enabled routed sources: device-owned priority, so
     * the core takes neither and preserves both edges. */
    {
        maskable_take_fixture fixture = {.program_slots = 1U,
                                         .program_addresses = {
                                             UINT32_C(0x1000)},
                                         .program_words = {NOP_WORD0}};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage,
                                    UINT32_C(0x1000));
        ti_c54x_run_result result;
        ti_c54x_state state = {0};

        seed_cpu(cpu, UINT16_C(0x0200));
        TI_C54X_TEST_CHECK(ti_c54x_mmr_file_write_imr(
                               &cpu->arch_state.mmr_file,
                               UINT16_C(0x0201)) ==
                           TI_C54X_MMR_FILE_OK);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                               cpu, HPINT_MASK, HPINT_VECTOR) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                               cpu, UINT16_C(0x0001),
                               UINT16_C(0x3100)) == TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                           UINT32_C(0x1001));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.ifr_value == UINT16_C(0x0201));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                           TI_C54X_ERROR_OK);
    }
}

static void
test_nmi_preempts_and_survives_maskable_wait(void)
{
    /* NMI wins over a pending maskable immediately ... */
    {
        maskable_take_fixture fixture = {0};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage,
                                    UINT32_C(0x1000));
        ti_c54x_run_result result;
        ti_c54x_state state = {0};

        seed_cpu(cpu, UINT16_C(0x0200));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                               cpu, HPINT_MASK, HPINT_VECTOR) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu,
                                                  UINT16_C(0x4000)) ==
                           TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                           UINT32_C(0x4000));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.ifr_value == HPINT_MASK);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                           TI_C54X_ERROR_OK);
    }
    /* ... and an NMI requested mid maskable-WAIT lands after
     * the maskable take retires, not eaten by its clear. */
    {
        maskable_take_fixture fixture = {.data_wait_then_ready = true};
        _Alignas(max_align_t) unsigned char storage[1024] = {0};
        ti_c54x_cpu *cpu = init_cpu(&fixture, storage,
                                    UINT32_C(0x1000));
        ti_c54x_run_result result;
        ti_c54x_state state = {0};

        seed_cpu(cpu, UINT16_C(0x0200));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                               cpu, HPINT_MASK, HPINT_VECTOR) ==
                           TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu,
                                                  UINT16_C(0x4000)) ==
                           TI_C54X_ERROR_OK);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_BUDGET_EXHAUSTED);
        /* The maskable take retired first: its edge
         * acknowledged, its vector taken. */
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                           UINT32_C(0xFFE4));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.ifr_value == UINT16_C(0x0000));
        TI_C54X_TEST_CHECK(state.nmi_pending);
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason ==
                           TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                           UINT32_C(0x4000));
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) ==
                           TI_C54X_ERROR_OK);
    }
}

static void
test_rete_round_trip_has_no_retake(void)
{
    maskable_take_fixture fixture = {.program_slots = 1U,
                                     .program_addresses = {
                                         UINT32_C(0xFFE4)},
                                     .program_words = {RETE_WORD0},
                                     .data_tos_value =
                                         UINT16_C(0x1000)};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;
    ti_c54x_state state = {0};

    seed_cpu(cpu, UINT16_C(0x0200));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                           cpu, HPINT_MASK, HPINT_VECTOR) ==
                       TI_C54X_ERROR_OK);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0xFFE4));
    /* The ISR returns through RETE with the acknowledged edge
     * gone: the parked program resumes instead of re-taking. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0200));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                        UINT16_C(0x0800)) == UINT16_C(0));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.ifr_value == UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_vector_arrival_rejections_reroute_losslessly();
    test_take_acknowledges_routes_and_sets_intm();
    test_masked_or_unknown_blocks_without_traffic();
    test_vectorless_and_priority_gaps_defer();
    test_nmi_preempts_and_survives_maskable_wait();
    test_rete_round_trip_has_no_retake();
    return 0;
}
