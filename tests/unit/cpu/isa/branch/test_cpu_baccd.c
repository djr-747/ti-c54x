/* Module contract
 * Owns: public-CPU RED proofs for the one-word `BACCD src`
 * delayed accumulator branch, including armed deferred takeover
 * to the accumulator target with two delay slots, sequential PC
 * advance past the slots, unknown-accumulator rejection without
 * state change or arming, T preservation, and full architectural
 * preservation outside PC.
 * Does not own: production CPU behavior, catalog/decode or pure
 * branch effects, address planning, shared fixtures, timing,
 * diagnostics, or test registration outside the branch leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/sequencer/delayed_branch/delayed_branch.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/arch_state.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct baccd_fixture {
    uint32_t base;
    uint16_t words[8];
    unsigned word_count;
    unsigned program_calls;
    unsigned data_calls;
} baccd_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    baccd_fixture *fixture = (baccd_fixture *)context;
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
    baccd_fixture *fixture = (baccd_fixture *)context;

    (void)phase;
    (void)request;
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
init_cpu(baccd_fixture *fixture, unsigned char *storage,
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

/* FORM-T2-12-02 delayed: 1111_011S_1110_0010; S selects A(0)/B(1). */
static uint16_t
baccd_word0(uint8_t select)
{
    return (uint16_t)(UINT16_C(0xF6E2) | ((uint16_t)select << 8U));
}

static void
seed_cpu(ti_c54x_cpu *cpu, uint64_t acc_a, uint64_t acc_b)
{
    cpu->arch_state.status.st0 = UINT16_C(0x0000);
    cpu->arch_state.status.st1 = UINT16_C(0x0000);
    cpu->arch_state.status.pmst = UINT16_C(0x0000);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_A,
                           acc_a) == TI_C54X_ACCUMULATOR_OK);
    TI_C54X_TEST_CHECK(ti_c54x_accumulator_write(
                           &cpu->accumulators, TI_C54X_ACCUMULATOR_B,
                           acc_b) == TI_C54X_ACCUMULATOR_OK);
    cpu->t_register.raw_value = UINT16_C(0x2468);
    cpu->t_register.known = true;
}

static void
check_armed_case(uint32_t start_pc, uint8_t select, uint64_t acc_a,
                 uint64_t acc_b, uint16_t expected_target)
{
    baccd_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;
    ti_c54x_arch_state saved_arch;
    ti_c54x_accumulator_state saved_accumulators;

    fixture.base = start_pc;
    fixture.words[0] = baccd_word0(select);
    fixture.words[1] = UINT16_C(0xF495);
    fixture.words[2] = UINT16_C(0xF495);
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, start_pc);
    seed_cpu(cpu, acc_a, acc_b);
    saved_arch = cpu->arch_state;
    saved_accumulators = cpu->accumulators;

    /* BACCD retires at once: PC advances sequentially past the
     * word and the deferred takeover arms to the accumulator
     * target with two delay-slot words remaining. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(1));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
    TI_C54X_TEST_CHECK(result.opcode_word == baccd_word0(select));
    TI_C54X_TEST_CHECK(result.opcode_valid);
    TI_C54X_TEST_CHECK(fixture.program_calls == 1U);
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == start_pc + UINT32_C(1));
    TI_C54X_TEST_CHECK(
        ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.words_remaining == UINT8_C(2));
    TI_C54X_TEST_CHECK(cpu->delayed_branch.deferred_pc_word ==
                       expected_target);
    TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
    TI_C54X_TEST_CHECK(cpu->t_register.raw_value == UINT16_C(0x2468));
    TI_C54X_TEST_CHECK(cpu->t_register.known);
    TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &saved_accumulators,
                              sizeof(saved_accumulators)) == 0);
    {
        ti_c54x_arch_state expect_arch = saved_arch;

        expect_arch.pc_word = start_pc + UINT32_C(1);
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &expect_arch,
                                  sizeof(expect_arch)) == 0);
    }

    /* Both delay slots retire, then the deferred branch takes
     * effect at the accumulator target. */
    result = ti_c54x_cpu_run(cpu, UINT64_C(2));
    TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
    TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(2));
    TI_C54X_TEST_CHECK(cpu->arch_state.pc_word ==
                       (uint32_t)expected_target);
    TI_C54X_TEST_CHECK(
        !ti_c54x_sequencer_delayed_branch_is_armed(&cpu->delayed_branch));
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_armed_vectors(void)
{
    /* TI Example 2: BACCD B with B=0x3000. */
    check_armed_case(UINT32_C(0x0025), UINT8_C(1),
                     UINT64_C(0x0000000000), UINT64_C(0x0000003000),
                     UINT16_C(0x3000));
    /* A select with guard bits set: low 16 bits only. */
    check_armed_case(UINT32_C(0x1000), UINT8_C(0),
                     UINT64_C(0xAB00FF00), UINT64_C(0x0000000001),
                     UINT16_C(0xFF00));
}

static void
test_unknown_accumulator_rejects_disarmed(void)
{
    baccd_fixture fixture = {0};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu;
    ti_c54x_run_result result;

    /* BACCD A with unknown A rejects without arming. */
    fixture.base = UINT32_C(0x1000);
    fixture.words[0] = baccd_word0(UINT8_C(0));
    fixture.words[1] = UINT16_C(0xF495);
    fixture.words[2] = UINT16_C(0xF495);
    fixture.word_count = 3U;
    cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    seed_cpu(cpu, UINT64_C(0x0000000001), UINT64_C(0x0000000002));
    cpu->accumulators.known_mask = (uint8_t)(cpu->accumulators.known_mask &
                                             (uint8_t)~UINT8_C(1));
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_acc = cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(fixture.data_calls == 0U);
        TI_C54X_TEST_CHECK(
            !ti_c54x_sequencer_delayed_branch_is_armed(
                &cpu->delayed_branch));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_acc,
                                  sizeof(before_acc)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

int
main(void)
{
    test_armed_vectors();
    test_unknown_accumulator_rejects_disarmed();
    return 0;
}
