/* Module contract
 *
 * Owns: public-CPU RED proofs for the one-word `FRETE`
 * interrupt-enabling far return, including the double-pop TOS
 * capture (extension page then return address), WAIT resume on
 * either leg with atomic commit, first- and second-leg DATA
 * fault transactionality, unknown-SP rejection, MMR TOS
 * rejection, single-word fetch ordering, and full architectural
 * preservation outside SP/PC/XPC/INTM.
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

typedef struct frete_fixture {
    uint16_t word;
    unsigned program_calls;
    ti_c54x_program_word_address address;
    bool program_fault;
    /* DATA endpoint script: WAIT once on the selected BEGIN leg,
     * then READY; or FAULT (optionally on the second BEGIN
     * only). The first BEGIN serves the extension page, the
     * second serves the return address. */
    bool data_wait_then_ready;
    bool data_wait_second_leg;
    bool data_fault;
    bool data_fault_second_leg_only;
    bool data_waited;
    uint16_t xpc_value;
    uint16_t pcl_value;
    uint16_t latched_value;
    ti_c54x_bus_word16_request data_requests[2];
    unsigned data_calls;
    unsigned begin_calls;
} frete_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    frete_fixture *fixture = (frete_fixture *)context;

    TI_C54X_TEST_CHECK(fixture->program_calls == 0U);
    TI_C54X_TEST_CHECK(value != NULL);
    if (fixture->program_fault) {
        return TI_C54X_BUS_FAULT;
    }
    fixture->address = address;
    *value = fixture->word;
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static void
advance_data(void *context, ti_c54x_bus_word16_advance_phase phase,
             uint64_t transaction_id,
             const ti_c54x_bus_word16_request *request,
             ti_c54x_bus_word16_observation *observation)
{
    frete_fixture *fixture = (frete_fixture *)context;
    bool wait_here = false;
    bool fault_here = false;

    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    if (phase == TI_C54X_BUS_WORD16_ADVANCE_BEGIN) {
        TI_C54X_TEST_CHECK(fixture->begin_calls < 2U);
        fixture->data_requests[fixture->begin_calls] = *request;
        fixture->begin_calls++;
        fixture->latched_value = (fixture->begin_calls == 1U)
                                     ? fixture->xpc_value
                                     : fixture->pcl_value;
        wait_here = fixture->data_wait_then_ready &&
                    ((fixture->begin_calls == 2U) ==
                     fixture->data_wait_second_leg);
        fault_here = fixture->data_fault &&
                     (!fixture->data_fault_second_leg_only ||
                      fixture->begin_calls == 2U);
    }
    fixture->data_calls++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
    if (fault_here) {
        observation->kind = TI_C54X_BUS_WORD16_OBSERVATION_FAULT;
        observation->bus_status = TI_C54X_BUS_FAULT;
        return;
    }
    if (wait_here && !fixture->data_waited) {
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
init_cpu(frete_fixture *fixture, unsigned char *storage, uint32_t reset_pc)
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

/* Table 2-15 far-return E=1 syntax: 1111_0100_1110_0101. */
static const uint16_t FRETE_WORD0 = UINT16_C(0xF4E5);

static frete_fixture
make_fixture(uint16_t xpc_value, uint16_t pcl_value)
{
    frete_fixture fixture = {0};

    fixture.word = FRETE_WORD0;
    fixture.xpc_value = xpc_value;
    fixture.pcl_value = pcl_value;
    return fixture;
}

static void
seed_cpu(ti_c54x_cpu *cpu, uint16_t sp_value, uint16_t xpc_value)
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
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_xpc(&cpu->arch_state,
                                                   xpc_value) ==
                       TI_C54X_ARCH_STATE_OK);
    /* CMPT/ARP stay unknown: pops are mode-independent end to end. */
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
    frete_fixture fixture = make_fixture(stack_xpc, stack_pcl);
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, start_pc);
    ti_c54x_run_result result;
    ti_c54x_state state;
    const uint16_t expected_sp =
        (uint16_t)(sp_before + UINT16_C(2));

    seed_cpu(cpu, sp_before, UINT16_C(0x0000));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;
        const ti_c54x_t_register_state before_t = cpu->t_register;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.opcode_word == FRETE_WORD0);
        TI_C54X_TEST_CHECK(result.opcode_valid);
        TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
        TI_C54X_TEST_CHECK(fixture.address == start_pc);
        /* The first TOS READ issues at old SP and returns the
         * extension page; the second issues at old SP + 1 and
         * returns the return address. */
        TI_C54X_TEST_CHECK(fixture.begin_calls == 2U);
        check_read_request(&fixture.data_requests[0], sp_before);
        check_read_request(&fixture.data_requests[1],
                           (uint16_t)(sp_before + UINT16_C(1)));
        TI_C54X_TEST_CHECK(result.pc_word == (uint32_t)stack_pcl);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                           (uint32_t)stack_pcl);
        TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_sp);
        {
            uint16_t xpc = UINT16_C(0xFFFF);

            TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(
                                   &cpu->arch_state, &xpc) ==
                               TI_C54X_ARCH_STATE_OK);
            TI_C54X_TEST_CHECK(xpc == stack_xpc);
        }
        /* The seed sets ST1 bit 11 (INTM); FRETE clears exactly it. */
        TI_C54X_TEST_CHECK((cpu->arch_state.status.st1 &
                           UINT16_C(0x0800)) == UINT16_C(0));
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 ==
                           UINT16_C(0x525A));
        TI_C54X_TEST_CHECK(cpu->arch_state.status.st1_known_mask ==
                           UINT16_C(0xFFFF));
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                  sizeof(before_t)) == 0);

        /* Only SP, the program counter, XPC, and INTM may move:
         * every other architectural field is pinned bit-exact.
         * The check is field-wise because the state struct
         * carries padding the optimizer leaves stale under -O3. */
        {
            size_t ar_index;

            TI_C54X_TEST_CHECK(cpu->arch_state.ifr == before_arch.ifr);
            TI_C54X_TEST_CHECK(cpu->arch_state.rc == before_arch.rc);
            TI_C54X_TEST_CHECK(cpu->arch_state.bk == before_arch.bk);
            TI_C54X_TEST_CHECK(cpu->arch_state.rtn == before_arch.rtn);
            for (ar_index = 0U; ar_index < 8U; ++ar_index) {
                TI_C54X_TEST_CHECK(cpu->arch_state.ar[ar_index] ==
                                   before_arch.ar[ar_index]);
            }
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st0 ==
                               before_arch.status.st0);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst ==
                               before_arch.status.pmst);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st0_known_mask ==
                               before_arch.status.st0_known_mask);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.st1_known_mask ==
                               before_arch.status.st1_known_mask);
            TI_C54X_TEST_CHECK(cpu->arch_state.status.pmst_known_mask ==
                               before_arch.status.pmst_known_mask);
            TI_C54X_TEST_CHECK(cpu->arch_state.mmr_file.imr ==
                               before_arch.mmr_file.imr);
            TI_C54X_TEST_CHECK(cpu->arch_state.mmr_file.trn ==
                               before_arch.mmr_file.trn);
            TI_C54X_TEST_CHECK(cpu->arch_state.mmr_file.brc ==
                               before_arch.mmr_file.brc);
            TI_C54X_TEST_CHECK(cpu->arch_state.mmr_file.rsa ==
                               before_arch.mmr_file.rsa);
            TI_C54X_TEST_CHECK(cpu->arch_state.mmr_file.rea ==
                               before_arch.mmr_file.rea);
            TI_C54X_TEST_CHECK(cpu->arch_state.mmr_file.known_mask ==
                               before_arch.mmr_file.known_mask);
            TI_C54X_TEST_CHECK(cpu->arch_state.swwsr.value ==
                               before_arch.swwsr.value);
            TI_C54X_TEST_CHECK(cpu->arch_state.swwsr.known ==
                               before_arch.swwsr.known);
            TI_C54X_TEST_CHECK(cpu->arch_state.clkmd.value ==
                               before_arch.clkmd.value);
            TI_C54X_TEST_CHECK(cpu->arch_state.clkmd.known ==
                               before_arch.clkmd.known);
            TI_C54X_TEST_CHECK(cpu->arch_state.scalar_known_mask ==
                               before_arch.scalar_known_mask);
            TI_C54X_TEST_CHECK(cpu->arch_state.ar_known_mask ==
                               before_arch.ar_known_mask);
        }
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == (uint32_t)stack_pcl);
        TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_worked_example(void)
{
    /* Inverse of the TI SPRU172C FCALL 013333h worked example: SP
     * 110F with TOS[110F] 0001 (saved XPC) and TOS[1110] 0027
     * (saved return) become PC 0027, XPC 0001, and SP 1111. */
    check_ready_case(UINT32_C(0x3333), UINT16_C(0x110F),
                     UINT16_C(0x0001), UINT16_C(0x0027));
}

static void
test_origins_and_targets(void)
{
    check_ready_case(UINT32_C(0x0000), UINT16_C(0x0020),
                     UINT16_C(0x0000), UINT16_C(0x0000));
    check_ready_case(UINT32_C(0x1F45), UINT16_C(0x8000),
                     UINT16_C(0x007F), UINT16_C(0x2000));
    /* SP wrap: old SP FFFE publishes 0000 across the two pops. */
    check_ready_case(UINT32_C(0x0100), UINT16_C(0xFFFE),
                     UINT16_C(0x0012), UINT16_C(0x00A5));
}

static void
check_wait_resumes_with_atomic_commit(bool second_leg)
{
    frete_fixture fixture = make_fixture(UINT16_C(0x0002),
                                         UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    ti_c54x_run_result result;

    fixture.data_wait_then_ready = true;
    fixture.data_wait_second_leg = second_leg;
    seed_cpu(cpu, UINT16_C(0x0300), UINT16_C(0x0000));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_WAIT);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0300));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x2112));
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == UINT16_C(0x5A5A));
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == UINT16_C(0x0302));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == UINT32_C(0x1000));
    TI_C54X_TEST_CHECK(cpu->arch_state.status.st1 == UINT16_C(0x525A));
    {
        uint16_t xpc = UINT16_C(0xFFFF);

        TI_C54X_TEST_CHECK(ti_c54x_arch_state_read_xpc(&cpu->arch_state,
                                                      &xpc) ==
                           TI_C54X_ARCH_STATE_OK);
        TI_C54X_TEST_CHECK(xpc == UINT16_C(0x0002));
    }
    TI_C54X_TEST_CHECK(fixture.begin_calls == 2U);
    check_read_request(&fixture.data_requests[0], UINT16_C(0x0300));
    check_read_request(&fixture.data_requests[1], UINT16_C(0x0301));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_wait_first_leg_resumes_with_atomic_commit(void)
{
    check_wait_resumes_with_atomic_commit(false);
}

static void
test_wait_second_leg_resumes_with_atomic_commit(void)
{
    check_wait_resumes_with_atomic_commit(true);
}

static void
check_fault_is_transactional(bool second_leg_only)
{
    frete_fixture fixture = make_fixture(UINT16_C(0x0002),
                                         UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    ti_c54x_run_result result;

    fixture.data_fault = true;
    fixture.data_fault_second_leg_only = second_leg_only;
    seed_cpu(cpu, UINT16_C(0x0300), UINT16_C(0x0000));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_first_leg_fault_is_transactional(void)
{
    check_fault_is_transactional(false);
}

static void
test_second_leg_fault_is_transactional(void)
{
    /* The stashed first-leg page never leaks: a second-leg FAULT
     * still retires nothing and moves no architecture. */
    check_fault_is_transactional(true);
}

static void
test_unknown_sp_rejects(void)
{
    frete_fixture fixture = make_fixture(UINT16_C(0x0002),
                                         UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    ti_c54x_run_result result;

    /* Seed everything except SP: reset leaves SP unknown. */
    cpu->arch_state.status.st0 = UINT16_C(0xA5A5);
    cpu->arch_state.status.st1 = UINT16_C(0x5A5A);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.begin_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
check_mmr_rejects(uint16_t sp_before)
{
    frete_fixture fixture = make_fixture(UINT16_C(0x0002),
                                         UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    ti_c54x_run_result result;

    seed_cpu(cpu, sp_before, UINT16_C(0x0000));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.begin_calls == 0U);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
    }
}

static void
test_mmr_first_tos_rejects(void)
{
    /* Old SP 0000 puts the first TOS at 0000 (IMR): an
     * MMR-routed first TOS has no endpoint first READ and
     * MMR+0 read semantics are unevidenced. */
    check_mmr_rejects(UINT16_C(0x0000));
}

static void
test_mmr_second_tos_rejects(void)
{
    /* Old SP FFFF puts the second TOS at 0000 (IMR) behind an
     * endpoint first TOS: the register would serve instead of
     * the endpoint, so the pop rejects the same way. */
    check_mmr_rejects(UINT16_C(0xFFFF));
}

static void
test_program_fault_is_transactional(void)
{
    frete_fixture fixture = make_fixture(UINT16_C(0x0002),
                                         UINT16_C(0x1000));
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x2112));
    ti_c54x_run_result result;

    fixture.program_fault = true;
    seed_cpu(cpu, UINT16_C(0x0300), UINT16_C(0x0000));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUS_FAULT);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int main(void)
{
    test_worked_example();
    test_origins_and_targets();
    test_wait_first_leg_resumes_with_atomic_commit();
    test_wait_second_leg_resumes_with_atomic_commit();
    test_first_leg_fault_is_transactional();
    test_second_leg_fault_is_transactional();
    test_unknown_sp_rejects();
    test_mmr_first_tos_rejects();
    test_mmr_second_tos_rejects();
    test_program_fault_is_transactional();
    puts("cpu_frete: ok");
    return 0;
}
