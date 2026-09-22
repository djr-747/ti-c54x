/* Module contract
 *
 * Owns: public-CPU RED proofs for the one-word `FRETD` delayed
 * far return, including the double-pop TOS capture (extension
 * page then return address) with sequential advance and
 * countdown arming, deferred-branch take effect with the armed
 * XPC already committed, WAIT resume staying armed, and full
 * architectural preservation outside SP/PC/XPC.
 * Does not own: production CPU behavior, catalog/decode or pure
 * return effects, address planning, shared fixtures, timing,
 * diagnostics, or test registration outside the branch leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/program_counter_extension.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct fretd_fixture {
    uint32_t base;
    uint16_t words[4];
    unsigned word_count;
    unsigned program_calls;
    bool data_wait_then_ready;
    bool data_waited;
    uint16_t xpc_value;
    uint16_t pcl_value;
    uint16_t latched_value;
    ti_c54x_bus_word16_request data_requests[2];
    unsigned begin_calls;
} fretd_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    fretd_fixture *fixture = (fretd_fixture *)context;
    uint32_t offset;

    TI_C54X_TEST_CHECK(value != NULL);
    TI_C54X_TEST_CHECK(address >= fixture->base);
    offset = address - fixture->base;
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
    fretd_fixture *fixture = (fretd_fixture *)context;

    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        TI_C54X_TEST_CHECK(fixture->begin_calls < 2U);
        fixture->data_requests[fixture->begin_calls] = *request;
        fixture->begin_calls++;
        fixture->latched_value = (fixture->begin_calls == 1U)
                                     ? fixture->xpc_value
                                     : fixture->pcl_value;
    }
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if (fixture->data_wait_then_ready &&
        phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN &&
        !fixture->data_waited) {
        fixture->data_waited = true;
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_WAIT;
        return;
    }
    observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_READY;
    observation->read_value_word = fixture->latched_value;
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
init_cpu(fretd_fixture *fixture, unsigned char *storage, uint32_t reset_pc)
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

/* Table 2-15 far-return Z=1 syntax: 1111_0110_1110_0100. */
static const uint16_t FRETD_WORD0 = UINT16_C(0xF6E4);
static const uint16_t NOP_WORD0 = UINT16_C(0xF495);

static void
seed_cpu(ti_c54x_cpu *cpu, uint16_t sp_value, uint16_t xpc_value)
{
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_xpc(&cpu->arch_state,
                                                   xpc_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           UINT64_C(0x1111222233)) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000AABBCC)) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
}

static void
check_read_request(const ti_c54x_bus_word16_request *request,
                   uint16_t address)
{
    TI_C54X_TEST_CHECK(request->space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(request->access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_READ);
    TI_C54X_TEST_CHECK(request->address_word == address);
}

static void
check_ready_case(uint32_t start_pc, uint16_t sp_before,
                 uint16_t stack_xpc, uint16_t stack_pcl)
{
    fretd_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    const uint16_t expected_sp =
        (uint16_t)(sp_before + UINT16_C(2));

    fixture.base = start_pc;
    fixture.words[0] = FRETD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    fixture.xpc_value = stack_xpc;
    fixture.pcl_value = stack_pcl;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_cpu(cpu, sp_before, UINT16_C(0x0000));

    /* FRETD retires first: both TOS READs issue at once with the
     * extension page and the return address, SP commits at once,
     * PC advances sequentially, XPC commits with the arming
     * retirement, and the countdown arms with the popped return
     * address deferred. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == FRETD_WORD0);
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_sp);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(1));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word == stack_pcl);
    TI_C54X_TEST_CHECK(fixture.begin_calls == 2U);
    check_read_request(&fixture.data_requests[0], sp_before);
    check_read_request(&fixture.data_requests[1],
                       (uint16_t)(sp_before + UINT16_C(1)));
    {
        uint16_t xpc = UINT16_C(0xFFFF);

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(&cpu->arch_state,
                                                      &xpc) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(xpc == stack_xpc);
    }

    /* Both delay slots retire, then the deferred branch takes
     * effect with the armed XPC already committed. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(2));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_sp);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)stack_pcl);
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    {
        uint16_t xpc = UINT16_C(0xFFFF);

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(&cpu->arch_state,
                                                      &xpc) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(xpc == stack_xpc);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_worked_example(void)
{
    /* FRETD with SP 110F: TOS[110F] 0001 (saved XPC) and
     * TOS[1110] 0027 (saved return) retire SP 1111 with PC
     * sequential and XPC 0001, then PC 0027 after the slots. */
    check_ready_case(UINT32_C(0x3001), UINT16_C(0x110F),
                     UINT16_C(0x0001), UINT16_C(0x0027));
}

static void
test_origins_and_targets(void)
{
    check_ready_case(UINT32_C(0x0000), UINT16_C(0x0020),
                     UINT16_C(0x0000), UINT16_C(0x0000));
    check_ready_case(UINT32_C(0x1F45), UINT16_C(0x8000),
                     UINT16_C(0x007F), UINT16_C(0x2000));
}

static void
test_wait_resumes_armed(void)
{
    fretd_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x3001);
    fixture.words[0] = FRETD_WORD0;
    fixture.words[1] = NOP_WORD0;
    fixture.words[2] = NOP_WORD0;
    fixture.word_count = 3U;
    fixture.xpc_value = UINT16_C(0x0001);
    fixture.pcl_value = UINT16_C(0x0027);
    fixture.data_wait_then_ready = true;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x3001));
    seed_cpu(cpu, UINT16_C(0x110F), UINT16_C(0x0000));

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x110F));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));

    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x1111));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x3002));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word ==
                       UINT16_C(0x0027));
    TI_C54X_TEST_CHECK(fixture.begin_calls == 2U);

    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x0027));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    test_worked_example();
    test_origins_and_targets();
    test_wait_resumes_armed();
    puts("cpu_fretd: ok");
    return 0;
}
