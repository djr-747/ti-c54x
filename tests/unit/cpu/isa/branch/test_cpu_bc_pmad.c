/* Module contract
 * Owns: public-CPU RED proofs for the two-word `BC pmad,cond`
 * conditional branch, including taken target commit and not-taken
 * sequential retirement from nonzero origins, the worked-example
 * vectors, two-word fetch ordering, unknown-input and second-word
 * fault transactionality, and full architectural preservation
 * without endpoint traffic.
 * Does not own: production CPU behavior, catalog/decode or pure
 * branch effects, address planning, shared fixtures, timing,
 * diagnostics, or test registration outside the branch leaf.
 */

#include <ti_c54x/cpu.h>

#include "../../../../../src/cpu/cpu_internal.h"
#include "../../../../../src/state/accumulator/accumulator.h"
#include "../../../../../src/state/register_layout.h"
#include "../../../../../src/state/status/registers.h"
#include "../../../../support/test_harness.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct bc_pmad_fixture {
    uint16_t words[2];
    unsigned program_calls;
    ti_c54x_program_word_address addresses[2];
    bool fail_second;
} bc_pmad_fixture;

static ti_c54x_bus_status
read_program(void *context, ti_c54x_program_word_address address,
             uint16_t *value)
{
    bc_pmad_fixture *fixture = (bc_pmad_fixture *)context;
    const unsigned index = fixture->program_calls;

    TI_C54X_TEST_CHECK(index < 2U);
    TI_C54X_TEST_CHECK(value != NULL);
    if (fixture->fail_second && index == 1U) {
        return TI_C54X_BUS_FAULT;
    }
    fixture->addresses[index] = address;
    *value = fixture->words[index];
    fixture->program_calls++;
    return TI_C54X_BUS_OK;
}

static ti_c54x_cpu *
init_cpu(bc_pmad_fixture *fixture, unsigned char *storage,
         uint32_t reset_pc)
{
    const ti_c54x_bus bus = {.context = fixture,
                             .program_read_word = read_program};
    const ti_c54x_config config = {reset_pc, {0U, NULL, NULL}};
    ti_c54x_cpu *cpu = NULL;

    TI_C54X_TEST_CHECK(ti_c54x_cpu_init(storage, ti_c54x_cpu_size(), &config,
                                        &bus, &cpu) == TI_C54X_ERROR_OK);
    return cpu;
}

/* FORM-T2-12-04: 1111_10Z0_CCCCCCCC with Z=0; word1 is the copied
 * raw pmad. */
static uint16_t
bc_pmad_word0(uint8_t cond)
{
    return (uint16_t)(UINT16_C(0xF800) | cond);
}

static void
seed_cpu(ti_c54x_cpu *cpu, uint64_t acc_a, uint64_t acc_b, uint8_t c,
         uint8_t tc)
{
    cpu->arch_state.status.st0 = UINT16_C(0x0000);
    cpu->arch_state.status.st1 = UINT16_C(0x0000);
    cpu->arch_state.status.pmst = UINT16_C(0x0000);
    cpu->arch_state.status.st0_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.st1_known_mask = UINT16_C(0xFFFF);
    cpu->arch_state.status.pmst_known_mask = UINT16_C(0xFFFF);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_c(
                           &cpu->arch_state.status, c) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_status_register_write_tc(
                           &cpu->arch_state.status, tc) ==
                       TI_C54X_STATUS_REGISTER_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_cmpt(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
    TI_C54X_TEST_CHECK(ti_c54x_arch_state_write_arp(&cpu->arch_state, 0U) ==
                       TI_C54X_ARCH_STATE_OK);
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
check_ready_case(uint32_t start_pc, uint8_t cond, uint16_t pmad,
                 uint64_t acc_a, uint64_t acc_b, uint8_t c, uint8_t tc,
                 uint32_t expected_pc)
{
    bc_pmad_fixture fixture = {.words = {bc_pmad_word0(cond), pmad},
                               .program_calls = 0U,
                               .addresses = {0U, 0U},
                               .fail_second = false};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, start_pc);
    ti_c54x_run_result result;
    ti_c54x_state state;

    seed_cpu(cpu, acc_a, acc_b, c, tc);
    {
        const ti_c54x_status_register_state before_status =
            cpu->arch_state.status;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;
        const ti_c54x_t_register_state before_t = cpu->t_register;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_BUDGET_EXHAUSTED);
        TI_C54X_TEST_CHECK(result.bus_status == TI_C54X_BUS_OK);
        TI_C54X_TEST_CHECK(result.instructions_attempted == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(1));
        TI_C54X_TEST_CHECK(result.opcode_word == bc_pmad_word0(cond));
        TI_C54X_TEST_CHECK(result.opcode_valid);
        TI_C54X_TEST_CHECK(fixture.program_calls == 2U);
        TI_C54X_TEST_CHECK(fixture.addresses[0] == start_pc);
        TI_C54X_TEST_CHECK(fixture.addresses[1] == start_pc + UINT32_C(1));
        TI_C54X_TEST_CHECK(result.pc_word == expected_pc);
        TI_C54X_TEST_CHECK(cpu->arch_state.pc_word == expected_pc);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->t_register, &before_t,
                                  sizeof(before_t)) == 0);

        /* Only the program counter may move. */
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state.status, &before_status,
                                  sizeof(before_status)) == 0);
        TI_C54X_TEST_CHECK(ti_c54x_cpu_inspect(cpu, &state) ==
                           TI_C54X_ERROR_OK);
        TI_C54X_TEST_CHECK(state.pc_word == expected_pc);
        TI_C54X_TEST_CHECK(state.instructions_retired_total == UINT64_C(1));
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_taken_and_not_taken(void)
{
    /* TC taken to an arbitrary target; NTC falls through to PC+2. */
    check_ready_case(UINT32_C(0x1000), UINT8_C(0x30), UINT16_C(0x2000),
                     UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1),
                     UINT32_C(0x2000));
    check_ready_case(UINT32_C(0x1000), UINT8_C(0x20), UINT16_C(0x2000),
                     UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1),
                     UINT32_C(0x1002));
    /* C taken; NC falls through. */
    check_ready_case(UINT32_C(0x1000), UINT8_C(0x0C), UINT16_C(0x3000),
                     UINT64_C(0), UINT64_C(0), UINT8_C(1), UINT8_C(0),
                     UINT32_C(0x3000));
    check_ready_case(UINT32_C(0x1000), UINT8_C(0x08), UINT16_C(0x3000),
                     UINT64_C(0), UINT64_C(0), UINT8_C(1), UINT8_C(0),
                     UINT32_C(0x1002));
    /* TI worked examples: BC 2000h,AGT from PC 1000. */
    check_ready_case(UINT32_C(0x1000), UINT8_C(0x46), UINT16_C(0x2000),
                     UINT64_C(0x53), UINT64_C(0), UINT8_C(0), UINT8_C(0),
                     UINT32_C(0x2000));
    check_ready_case(UINT32_C(0x1000), UINT8_C(0x46), UINT16_C(0x2000),
                     UINT64_C(0xFFFFFFFFFF), UINT64_C(0), UINT8_C(0),
                     UINT8_C(0), UINT32_C(0x1002));
    /* Degenerate target equal to the sequential successor still takes
     * the branch route. */
    check_ready_case(UINT32_C(0x1000), UINT8_C(0x30), UINT16_C(0x1002),
                     UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1),
                     UINT32_C(0x1002));
}

static void
test_unknown_input_rejects(void)
{
    bc_pmad_fixture fixture = {.words = {bc_pmad_word0(UINT8_C(0x30)),
                                         UINT16_C(0x2000)},
                               .program_calls = 0U,
                               .addresses = {0U, 0U},
                               .fail_second = false};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1));
    cpu->arch_state.status.st0_known_mask = (uint16_t)(
        cpu->arch_state.status.st0_known_mask &
        (uint16_t)~TI_C54X_ST0_TC_MASK);
    {
        const ti_c54x_arch_state before_arch = cpu->arch_state;
        const ti_c54x_accumulator_state before_accumulators =
            cpu->accumulators;

        result = ti_c54x_cpu_run(cpu, UINT64_C(1));
        TI_C54X_TEST_CHECK(result.reason == TI_C54X_STOP_INVALID_STATE);
        TI_C54X_TEST_CHECK(result.instructions_retired == UINT64_C(0));
        TI_C54X_TEST_CHECK(memcmp(&cpu->arch_state, &before_arch,
                                  sizeof(before_arch)) == 0);
        TI_C54X_TEST_CHECK(memcmp(&cpu->accumulators, &before_accumulators,
                                  sizeof(before_accumulators)) == 0);
    }
    TI_C54X_TEST_CHECK(ti_c54x_cpu_deinit(cpu) == TI_C54X_ERROR_OK);
}

static void
test_second_word_fault_is_transactional(void)
{
    bc_pmad_fixture fixture = {.words = {bc_pmad_word0(UINT8_C(0x30)),
                                         UINT16_C(0x2000)},
                               .program_calls = 0U,
                               .addresses = {0U, 0U},
                               .fail_second = true};
    _Alignas(max_align_t) unsigned char storage[1024] = {0};
    ti_c54x_cpu *cpu = init_cpu(&fixture, storage, UINT32_C(0x1000));
    ti_c54x_run_result result;

    seed_cpu(cpu, UINT64_C(0), UINT64_C(0), UINT8_C(0), UINT8_C(1));
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
    test_taken_and_not_taken();
    test_unknown_input_rejects();
    test_second_word_fault_is_transactional();
    return 0;
}
