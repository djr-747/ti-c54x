/* Module contract
 * Owns: public-CPU RED proof for the P3-S96 HPINT hop: the
 * six-retirement composition of a parked post-CALL program
 * (INTM set, HPINT routed to 0x3772 and pending but masked)
 * with an NMI to a RETF stub, the stub clearing INTM with
 * the edge preserved, the maskable take to the routed entry
 * with its acknowledge, the RETE resume, and one parked step
 * with no re-take.
 * Does not own: production CPU behavior, catalog/decode or
 * pure take effects, address planning, shared fixtures, the
 * single-step take proofs (S94/S95 leaves), multi-source
 * priority, timing, diagnostics, or test registration
 * outside the status leaf.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint16_t CALL_WORD0 = UINT16_C(0xF074);
static const uint16_t NOP_WORD0 = UINT16_C(0xF495);
static const uint16_t RETF_WORD0 = UINT16_C(0xF49B);
static const uint16_t RETE_WORD0 = UINT16_C(0xF4EB);
/* Loader2 doorbell flavor: HPINT mask, RETF stub, loader2 entry. */
static const uint16_t HPINT_MASK = UINT16_C(0x0200);
static const uint16_t STUB_VECTOR = UINT16_C(0x3100);
static const uint16_t LOADER2_ENTRY = UINT16_C(0x3772);
static const uint16_t PARKED_PC = UINT16_C(0x1002);

typedef struct hpint_hop_fixture {
    uint32_t program_addresses[8];
    uint16_t program_words[8];
    unsigned program_slots;
    unsigned program_calls;
    uint16_t data_tos_value;
    ti_c54x_bus_word16_request data_request;
    unsigned data_calls;
} hpint_hop_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    hpint_hop_fixture *fixture = (hpint_hop_fixture *)context;
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
    hpint_hop_fixture *fixture = (hpint_hop_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->data_request = *request;
    fixture->data_calls++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
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
init_cpu(hpint_hop_fixture *fixture, unsigned char *storage,
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

/* All-known status with INTM set (ST1 bit 11 = 1): the parked
 * program runs masked. SP seeded known; IMR seeded known with
 * exactly the HPINT bit enabled. RTN stays unknown until the
 * CALL stages it. */
static void
seed_cpu(ti_c54x_cpu *cpu, uint16_t sp_value)
{
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
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
test_hpint_hop_nmi_retf_then_take_then_rete(void)
{
    hpint_hop_fixture fixture = {
        .program_slots = 6U,
        .program_addresses = {UINT32_C(0x1000), UINT32_C(0x1001),
                              UINT32_C(0x1002), UINT32_C(0x1003),
                              UINT32_C(0x3100), UINT32_C(0x3772)},
        .program_words = {CALL_WORD0, PARKED_PC, NOP_WORD0, NOP_WORD0,
                          RETF_WORD0, RETE_WORD0},
        /* The RETE pop is the only stack READ; it must see the
         * parked PC the HPINT take pushed. */
        .data_tos_value = PARKED_PC};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;
    ti_c54x_state state = {0};

    seed_cpu(cpu, UINT16_C(0x0200));
    TI_C54X_TEST_CHECK((cpu->arch_state.scalar_known_mask &
                        TI_C54X_ARCH_STATE_KNOWN_RTN) == 0U);

    /* HPINT routes to loader2 while masked; the edge latches
     * with no bus traffic. The NMI arrives only after the
     * park is established (it would preempt decode at once). */
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_interrupt_vector(
                           cpu, HPINT_MASK, LOADER2_ENTRY) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.ifr_value == HPINT_MASK);
    TI_C54X_TEST_CHECK(!state.nmi_pending);
    TI_C54X_TEST_CHECK(fixture.program_calls == 0U);
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);

    /* 1. The parked CALL retires and stages RTN for the stub. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* Multiword CALL reports its word without opcode_valid. */
    TI_C54X_TEST_CHECK(result.opcode_word == CALL_WORD0);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                       UINT32_C(0x1002));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x01FF));
    TI_C54X_TEST_CHECK(cpu->arch_state.rtn == PARKED_PC);
    TI_C54X_TEST_CHECK((cpu->arch_state.scalar_known_mask &
                        TI_C54X_ARCH_STATE_KNOWN_RTN) != 0U);
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                        UINT16_C(0x0800)) != UINT16_C(0));
    TI_C54X_TEST_CHECK(fixture.data_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                       UINT16_C(0x01FF));
    TI_C54X_TEST_CHECK(fixture.data_request.write_value_word ==
                       PARKED_PC);
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);

    /* 2. The NMI names the RETF stub and preempts the masked
     * park: parked PC pushes, no stub fetch, the NMI edge
     * consumes, HPINT stays. */
    TI_C54X_TEST_CHECK(ti_c54x_cpu_request_nmi(cpu, STUB_VECTOR) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.nmi_pending);
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    /* opcode_valid still reflects the CALL fetch: the take
     * itself fetches nothing. */
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                       UINT32_C(0x3100));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x01FE));
    TI_C54X_TEST_CHECK(fixture.data_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                       UINT16_C(0x01FE));
    TI_C54X_TEST_CHECK(fixture.data_request.write_value_word ==
                       PARKED_PC);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(!state.nmi_pending);
    TI_C54X_TEST_CHECK(state.ifr_value == HPINT_MASK);

    /* 3. The stub RETF takes PC from RTN, discards the pushed
     * slot with no bus traffic, clears INTM, and preserves
     * the HPINT edge. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == RETF_WORD0);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                       UINT32_C(0x1002));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x01FF));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                        UINT16_C(0x0800)) == UINT16_C(0));
    TI_C54X_TEST_CHECK(fixture.data_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.program_calls == 3U);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.ifr_value == HPINT_MASK);

    /* 4. Unmasked, the pending HPINT takes to loader2: parked
     * PC pushes, exactly its IFR bit acknowledges, INTM sets. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.program_calls == 3U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                       UINT32_C(0x3772));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x01FE));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                        UINT16_C(0x0800)) != UINT16_C(0));
    TI_C54X_TEST_CHECK(fixture.data_calls == 3U);
    TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                       UINT16_C(0x01FE));
    TI_C54X_TEST_CHECK(fixture.data_request.write_value_word ==
                       PARKED_PC);
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.ifr_value == UINT16_C(0x0000));

    /* 5. Loader2 returns through RETE: the parked PC pops,
     * INTM clears, and the acknowledged edge stays gone. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == RETE_WORD0);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                       UINT32_C(0x1002));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x01FF));
    TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                        UINT16_C(0x0800)) == UINT16_C(0));
    TI_C54X_TEST_CHECK(fixture.data_calls == 4U);
    TI_C54X_TEST_CHECK(fixture.data_request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(fixture.data_request.address_word ==
                       UINT16_C(0x01FE));

    /* 6. The parked program resumes instead of re-taking. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(result.opcode_word == NOP_WORD0);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                       UINT32_C(0x1003));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                       TI_C54X_ERROR_OK);
    TI_C54X_TEST_CHECK(state.ifr_value == UINT16_C(0x0000));
    TI_C54X_TEST_CHECK(!state.nmi_pending);
    TI_C54X_TEST_CHECK(state.instructions_retired_total ==
                       UINT64_C(6));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_hpint_hop_nmi_retf_then_take_then_rete();
    return 0;
}
