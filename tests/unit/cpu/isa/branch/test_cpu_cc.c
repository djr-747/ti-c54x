/* Module contract
 * Owns: public-CPU RED proofs for the two-word `CC pmad,cond`
 * non-delayed conditional call, including taken immediate call
 * with the PC+2 TOS WRITE, SP commit, and direct PC=pmad
 * retirement with no countdown, not-taken sequential
 * retirement with no endpoint traffic, unknown-condition and
 * taken-with-unknown-SP rejection without state change,
 * not-taken retirement with unknown SP, and full architectural
 * preservation outside SP/PC.
 * Does not own: production CPU behavior, catalog/decode or pure
 * call effects, address planning, shared fixtures, timing,
 * diagnostics, or test registration outside the branch leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct cc_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
    ti_c54x_bus_word16_request data_request;
    unsigned data_calls;
} cc_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    cc_fixture *fixture = (cc_fixture *)context;
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
    cc_fixture *fixture = (cc_fixture *)context;

    (void)phase;
    TI_C54X_TEST_CHECK(request != NULL);
    TI_C54X_TEST_CHECK(observation != NULL);
    fixture->data_request = *request;
    fixture->data_calls++;
    *observation = (ti_c54x_bus_word16_observation){0};
    observation->transaction_id = transaction_id;
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

static ti_c54x_cpu *
init_cpu(cc_fixture *fixture, unsigned char *storage,
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

/* FORM-T2-13-03: 1111_1001_CCCCCCCC with Z=0; word1 is the copied
 * raw pmad. */
static uint16_t
cc_word0(uint8_t cond)
{
    return (uint16_t)(UINT16_C(0xF900) | cond);
}

static void
seed_cpu(ti_c54x_cpu *cpu, uint16_t sp_value, uint64_t acc_a,
         uint8_t tc)
{
    cpu->arch_state.status.st0 = UINT16_C(0x0000);
    cpu->arch_state.status.st1 = UINT16_C(0x0000);
    cpu->arch_state.status.pmst = UINT16_C(0x0000);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_sp(&cpu->arch_state,
                                                  sp_value) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           acc_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           UINT64_C(0x0000AABBCC)) ==
                       TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(
                           &cpu->arch_state.status, tc) ==
                       TI_C54X_STATUS_REGISTER_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
}

static void
check_taken_case(uint32_t start_pc, uint8_t cond, uint16_t sp_before,
                 uint16_t pmad)
{
    cc_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    const uint16_t expected_tos = (uint16_t)(sp_before - UINT16_C(1));
    const uint16_t expected_return = (uint16_t)(start_pc + UINT32_C(2));
    ti_c54x_arch_state saved_arch;
    ti_c54x_accumulator_state saved_accumulators;

    fixture.base = start_pc;
    fixture.words[0] = cc_word0(cond);
    fixture.words[1] = pmad;
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, start_pc);
    /* TC=1 with A nonzero takes every evidenced condition below
     * except NTC/C clear shapes, which the per-condition cases
     * select explicitly. */
    seed_cpu(cpu, sp_before, UINT64_C(0x0000000001), UINT8_C(1));
    saved_arch = cpu->arch_state;
    saved_accumulators = cpu->accumulators;

    /* Taken CC retires at once per the CALL convention: SP commits
     * with the PC+2 TOS WRITE, PC jumps straight to pmad, and no
     * countdown arms. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == cc_word0(cond));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(cpu->arch_state.sp == expected_tos);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == (uint32_t)pmad);
    TI_C54X_TEST_CHECK(result.pc_word == (uint32_t)pmad);
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(fixture.data_calls == 1U);
    TI_C54X_TEST_CHECK(fixture.data_request.space ==
                       TI_C54X_BUS_WORD16_REQUEST_SPACE_DATA);
    TI_C54X_TEST_CHECK(fixture.data_request.access ==
                       TI_C54X_BUS_WORD16_REQUEST_ACCESS_WRITE);
    TI_C54X_TEST_CHECK(fixture.data_request.address_word == expected_tos);
    TI_C54X_TEST_CHECK(fixture.data_request.write_value_word ==
                       expected_return);
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &saved_accumulators,
                              sizeof(saved_accumulators)) == 0);
    {
        ti_c54x_arch_state expect_arch = saved_arch;

        expect_arch.pc_word = (uint32_t)pmad;
        expect_arch.sp = expected_tos;
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &expect_arch,
                                  sizeof(expect_arch)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_taken_calls_at_once(void)
{
    /* TC takes with the TOS WRITE of PC+2. */
    check_taken_case(UINT32_C(0x0025), UINT8_C(0x30), UINT16_C(0x1111),
                     UINT16_C(0x1000));
    /* AGT on A=1 takes with a distinct target. */
    check_taken_case(UINT32_C(0x1000), UINT8_C(0x46), UINT16_C(0x8000),
                     UINT16_C(0x2000));
}

static void
check_not_taken_case(uint32_t start_pc, uint8_t cond, uint64_t acc_a,
                     uint8_t tc, uint16_t sp_before, uint16_t pmad)
{
    cc_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_arch_state saved_arch;
    ti_c54x_accumulator_state saved_accumulators;

    fixture.base = start_pc;
    fixture.words[0] = cc_word0(cond);
    fixture.words[1] = pmad;
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_cpu(cpu, sp_before, acc_a, tc);
    saved_arch = cpu->arch_state;
    saved_accumulators = cpu->accumulators;

    /* Not-taken CC retires sequentially with no countdown and no
     * endpoint traffic. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == cc_word0(cond));
    TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK(result.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(2));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &saved_accumulators,
                              sizeof(saved_accumulators)) == 0);
    {
        ti_c54x_arch_state expect_arch = saved_arch;

        expect_arch.pc_word = start_pc + UINT32_C(2);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &expect_arch,
                                  sizeof(expect_arch)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_not_taken_falls_through(void)
{
    /* TC clear does not take. */
    check_not_taken_case(UINT32_C(0x1000), UINT8_C(0x30),
                         UINT64_C(0x0000000000), UINT8_C(0),
                         UINT16_C(0x1111), UINT16_C(0x2000));
    /* ANEQ on zero A does not take. */
    check_not_taken_case(UINT32_C(0x1000), UINT8_C(0x44),
                         UINT64_C(0x0000000000), UINT8_C(0),
                         UINT16_C(0x8000), UINT16_C(0x1000));
}

static void
test_unknown_tc_rejects(void)
{
    cc_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x1000);
    fixture.words[0] = cc_word0(UINT8_C(0x30));
    fixture.words[1] = UINT16_C(0x2000);
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT16_C(0x1111), UINT64_C(0x0000000001), UINT8_C(1));
    cpu->arch_state.status.st0_known_mask = UINT16_C(0x0000);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(
            !ti_c54x_sequencer_delayed_branch_is_armed(
                &cpu->delayed_branch));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_sp_taken_rejects(void)
{
    cc_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x1000);
    fixture.words[0] = cc_word0(UINT8_C(0x30));
    fixture.words[1] = UINT16_C(0x2000);
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT16_C(0x1111), UINT64_C(0x0000000001), UINT8_C(1));
    cpu->arch_state.scalar_known_mask = (uint8_t)(
        cpu->arch_state.scalar_known_mask &
        (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_SP);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        /* Taken with unknown SP rejects per the CALL convention. */
        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(
            !ti_c54x_sequencer_delayed_branch_is_armed(
                &cpu->delayed_branch));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_unknown_sp_not_taken_retires(void)
{
    cc_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    fixture.base = UINT32_C(0x1000);
    fixture.words[0] = cc_word0(UINT8_C(0x30));
    fixture.words[1] = UINT16_C(0x2000);
    fixture.word_count = 2U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT16_C(0x1111), UINT64_C(0x0000000000), UINT8_C(0));
    cpu->arch_state.scalar_known_mask = (uint8_t)(
        cpu->arch_state.scalar_known_mask &
        (uint8_t)~TI_C54X_ARCH_STATE_KNOWN_SP);

    /* Not-taken consumes condition inputs only: unknown SP retires. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK(result.pc_word == UINT32_C(0x1002));
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_taken_calls_at_once();
    test_not_taken_falls_through();
    test_unknown_tc_rejects();
    test_unknown_sp_taken_rejects();
    test_unknown_sp_not_taken_retires();
    return 0;
}
